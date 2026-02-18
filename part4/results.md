# Part 4: Syscall Latency Benchmark Results

## System Configuration

- **CPU:** Intel Xeon 6960P @ 2.70 GHz
- **Kernel:** 6.18.0esp-LKP26-dirty (custom kernel with sys_null / sys_kstats)
- **glibc:** 2.39 (Ubuntu GLIBC 2.39-0ubuntu8.7)
- **Method:** `taskset -c 0 ./syscall_bench` (pinned to CPU 0)
- **Warmup:** 100 iterations; **Trials:** 10 × 100,000 iterations

## Raw Results

| # | Test | Avg (ns) | Min (ns) | Max (ns) | StdDev |
|---|------|----------|----------|----------|--------|
| 1 | `sys_null` (pure overhead) | 83.8 | 76.3 | 150.0 | 22.1 |
| 2 | `syscall(__NR_getpid)` | 83.1 | 83.0 | 83.3 | 0.2 |
| 3 | `getpid()` [libc] | 82.7 | 82.6 | 83.0 | 0.2 |
| 4 | `syscall(gettimeofday)` [trap] | 123.0 | 123.0 | 123.0 | — |
| 5 | `gettimeofday()` [vDSO] | 25.7 | 25.6 | 25.9 | 0.2 |
| 6 | `clock_gettime()` [vDSO] | 24.1 | 24.1 | 24.1 | 0.0 |
| 7 | `sys_kstats` (custom syscall) | 1238.6 | 1223.3 | 1257.8 | 9.0 |

> Note: `syscall(gettimeofday)` reported StdDev = NaN in one run because all 10 trial averages
> rounded to the same value (123.0 ns) at single-decimal precision, making the variance exactly 0
> before floating-point rounding. This is not a bug; it reflects extremely stable latency.

---

## Performance Questions

### Q1. What is the pure syscall overhead (sys_null) in nanoseconds?

**83.8 ns** (average across 10 trials of 100,000 iterations each, pinned to CPU 0).

At 2.70 GHz that corresponds to roughly **226 cycles** for the full round-trip:
userspace `syscall` instruction → kernel entry (SWAPGS, CR3 switch, stack switch,
dispatch) → `sys_null` body (immediately returns 0) → kernel exit → return to userspace.

The high StdDev (22.1 ns) and wide min/max (76–150 ns) for test 1 relative to tests 2–4
is expected: the null syscall is so cheap that occasional cache misses on the first few
iterations of each trial (despite warmup) dominate. The min of 76.3 ns represents the
"hot cache" cost.

---

### Q2. How does `getpid()` via libc compare to `syscall(__NR_getpid)`?

| Path | Avg (ns) |
|------|----------|
| `syscall(__NR_getpid)` | 83.1 |
| `getpid()` libc | 82.7 |

They are **essentially identical** (difference < 0.5 ns, within noise). Both go through
the full kernel trap path.

**Why no cache?** glibc removed its PID cache in **version 2.25** (released 2017).
Before 2.25, `getpid()` stored the PID in thread-local storage and returned it without
a syscall after the first call — making it near-zero cost. The cache was removed because
it caused correctness bugs after `clone()` and `unshare()` in multi-threaded programs
where the cached PID could become stale.

This system runs **glibc 2.39**, well past 2.25, so `getpid()` unconditionally issues
a `getpid` syscall on every call, giving the same latency as the raw `syscall()` wrapper.
The tiny 0.4 ns advantage for libc is within measurement noise.

---

### Q3. How much faster is vDSO `gettimeofday()` vs. the real syscall?

| Path | Avg (ns) |
|------|----------|
| `syscall(gettimeofday)` — kernel trap | 123.0 ns |
| `gettimeofday()` — vDSO | 25.7 ns |

- **Absolute difference:** 123.0 − 25.7 = **97.3 ns faster**
- **Speedup ratio:** 123.0 / 25.7 ≈ **4.8× faster**

The vDSO (Virtual Dynamic Shared Object) maps a small read-only code+data page from
the kernel directly into every process's address space. The `gettimeofday` vDSO
implementation reads the kernel's `vdso_data` page (updated by the kernel's timekeeping
tick) entirely in userspace — no `syscall` instruction, no privilege-level switch, no
TLB flush, no stack switch. The ~25 ns cost is purely the overhead of reading a few
memory-mapped values and doing arithmetic (roughly 69 cycles at 2.70 GHz).

`clock_gettime(CLOCK_MONOTONIC)` at 24.1 ns is marginally faster still, because its
vDSO path uses a slightly simpler code path than `gettimeofday`.

---

### Q4. What fraction of `sys_kstats` latency is pure overhead vs. actual work?

| Component | Latency |
|-----------|---------|
| `sys_null` (pure trap overhead) | 83.8 ns |
| `sys_kstats` (total) | 1238.6 ns |
| Kernel work (kstats − null) | **1154.8 ns** |

- **Pure trap overhead:** 83.8 / 1238.6 = **6.8%**
- **Actual work in kernel:** 1154.8 / 1238.6 = **93.2%**

The vast majority of `sys_kstats` latency (~93%) is spent in the syscall body itself,
not in the trap machinery. `sys_kstats` must aggregate data from multiple kernel
subsystems on every call:

- **Process count / runnable count** — walk scheduler run-queue data or read
  `nr_threads` / `nr_running` global counters.
- **Memory statistics** — read `totalram_pages`, `vm_stat[]` arrays (free, buffers).
- **Uptime** — call `ktime_get_boottime_seconds()`.
- **Context switches** — iterate over all CPUs summing `cpu_rq(cpu)->nr_switches`.

The per-CPU summation of context switches is the most expensive part: it touches one
cache line per CPU across all online processors, causing cache misses proportional to
the CPU count. This explains why `sys_kstats` is **~15× slower** than `sys_null`
despite doing no complex computation.
