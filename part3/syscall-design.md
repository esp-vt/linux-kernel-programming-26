# sys_kstats Syscall Design Analysis

## 1. Why a struct rather than multiple output arguments or multiple syscalls?

### Multiple output arguments

The x86-64 Linux syscall ABI passes arguments in six registers: `rdi`, `rsi`,
`rdx`, `r10`, `r8`, `r9`. `sys_kstats` returns seven fields. Multiple output
pointer arguments would exceed the register limit before all fields are covered,
requiring a wrapper struct anyway.

Beyond the register limit, each pointer argument demands its own `access_ok` check
and `copy_to_user` call. Seven separate copies means seven opportunities to return
`-EFAULT` mid-way, leaving the caller with a partially filled result and no clean
way to know which fields were written. A single struct pointer collapses all of
that into one check and one copy.

Finally, a struct can be versioned. Adding a `size` parameter alongside the struct
pointer lets a newer kernel detect an older userspace binary and fill only as many
bytes as the binary expects. Individual pointer arguments offer no equivalent
extensibility path.

### Multiple syscalls

A separate syscall per field widens the observation window from microseconds (one
syscall body) to milliseconds (N round-trips through the syscall dispatcher). That
directly worsens atomicity and adds N times the context-switch and kernel-entry
overhead. From the caller's perspective it also requires N error checks and the
values cannot be treated as a consistent group. One struct/one syscall is strictly
better on every axis.

---

## 2. The exception table mechanism: what happens when copy_to_user() gets a bad address?

The implementation ends with:

```c
if (copy_to_user(stats, &ks, sizeof(ks)))
    return -EFAULT;
```

There are two distinct code paths depending on _why_ the address is invalid.

### Path A: access_ok() fast reject

`copy_to_user` first calls `access_ok(stats, sizeof(ks))`, which checks that the
entire range `[stats, stats + sizeof(ks))` lies below `TASK_SIZE_MAX` (~128 TiB on
x86-64). An obviously out-of-range address — one past the top of the user virtual
address space — fails here immediately in software, before any memory access. The
function returns nonzero, `copy_to_user` returns the byte count uncopied, and
`sys_kstats` returns `-EFAULT`. No hardware fault is raised.

### Path B: hardware page fault → exception table fixup

If `access_ok` passes (the address is numerically in range) but the virtual page is
not mapped — for example, userspace passed a valid-looking but unmapped address, or
NULL whose page is deliberately left unmapped — the CPU raises a **#PF (vector 14)**
when the copy instruction (`rep movsb` or equivalent) touches the address.

Step-by-step:

1. **CPU raises #PF** in ring 0 (the fault occurs while executing kernel code during
   `copy_to_user`). The CPU saves `RIP`, `RSP`, `RFLAGS`, `CS`, `SS` on the kernel
   stack and sets `CR2` to the faulting virtual address.

2. **`do_page_fault` / `exc_page_fault` is called.** The handler inspects `CS.CPL`:
   the fault happened at CPL=0 (kernel mode), so it cannot be a normal userspace
   page fault.

3. **`fixup_exception` searches `__ex_table`.** The kernel maintains a read-only
   section called `__ex_table` (the exception table), built at compile time by the
   `EX_TABLE` macro that annotates every potentially-faulting instruction inside
   `copy_to_user`, `get_user`, `put_user`, etc. Each entry is a pair:
   `(faulting_insn_address, fixup_address)`. `search_exception_tables(regs->ip)`
   performs a binary search for the saved `RIP`.

4. **Fixup is found.** The handler redirects execution by overwriting `regs->ip`
   with the fixup address and returning from the fault handler. No oops, no panic.

5. **The fixup routine runs.** For `copy_to_user` the fixup typically:
   - Sets `%rax` to the number of bytes not copied (nonzero signals failure).
   - Zeroes any destination bytes that were not yet written (so userspace never
     sees stale kernel stack contents).
   - Falls through to the normal function return path.

6. **`copy_to_user` returns nonzero** to `sys_kstats`, which returns `-EFAULT` to
   userspace. The test program sees `errno = 14` (`EFAULT`), which is exactly what
   the output shows for the NULL-pointer test.

If `__ex_table` did not contain an entry for the faulting instruction — for
instance, if a developer wrote a raw pointer dereference instead of `copy_to_user`
— the fault handler would find no fixup, declare an unrecoverable kernel fault, and
oops (or panic if `panic_on_oops=1`). The exception table is what makes safe
kernel↔user memory transfers possible.

---

## 3. Are all returned fields from the same instant in time?

No. The implementation reads fields sequentially, with no global lock held across
the entire sequence:

```c
ks.nr_processes      = nr_processes();        // reads nr_threads (atomic_t)
ks.nr_running        = nr_running();          // sums per-CPU rq->nr_running
ks.context_switches  = nr_context_switches(); // sums per-CPU counters

si_meminfo(&si);                              // reads globalzone page-state counters
ks.total_memory_kb   = (si.totalram  * si.mem_unit) / 1024;
ks.free_memory_kb    = (si.freeram   * si.mem_unit) / 1024;
ks.buffer_memory_kb  = (si.bufferram * si.mem_unit) / 1024;

ks.uptime_seconds    = ktime_get_boottime_seconds(); // reads monotonic clock
```

### What can change between consecutive calls?

| Transition | What races |
|---|---|
| Between `nr_processes` and `nr_running` | A `fork()` completes: `nr_processes` is pre-fork, `nr_running` is post-fork. |
| During `nr_running` aggregation | The scheduler migrates a task between CPUs while the per-CPU loop is still running; the migrating task is counted zero or twice. |
| Between `nr_context_switches` and `si_meminfo` | Any context switch increments per-CPU counters; the aggregation loop is not atomic across CPUs. |
| Within `si_meminfo` itself | The page allocator on another CPU can alloc or free pages between the `freeram` read and the `bufferram` read, producing a pair of values that never coexisted. |
| Between `si_meminfo` and `ktime_get_boottime_seconds` | The one-second jiffy tick advances; uptime is from a slightly later instant than memory. |

### Does it matter for a statistics reporting syscall?

For this use case, no — with the following reasoning:

- **Expected consumer.** A monitoring tool or `/proc`-style display wants
  approximate, human-readable figures. True atomic consistency would require
  either stopping all CPUs (prohibitively expensive) or reading from a pre-built
  snapshot (added complexity). Neither is warranted for statistics.

- **The window is narrow.** All seven reads happen inside one non-preemptible
  kernel function call. The observation window is measured in hundreds of
  nanoseconds, far smaller than the millisecond-scale window of reading three
  separate `/proc` files in sequence (as the cross-check test does).

- **The cross-check confirms this.** Context switches diverged by exactly 2
  between `sys_kstats` and the subsequent `/proc/stat` read — demonstrating that
  the syscall's internal window is negligible compared to the I/O round-trip time.

If strict per-field consistency were required (e.g., for accounting or billing),
the kernel would need to collect values from a pre-locked snapshot, which is
outside the scope of a general-purpose stats call.

---

## 4. Syscall vs. /proc: atomicity, latency, and maintainability

### Atomicity

| Aspect | `/proc` approach | `sys_kstats` |
|---|---|---|
| Cross-field consistency | Each file is an independent snapshot at a different time T₁, T₂, T₃, … | Single kernel execution; all fields within one microsecond-scale window. |
| Within a single file | The `->show()` callback for a proc file holds no locks across the whole output, so it too can be internally inconsistent for large files. | Same lock-free reads, but a much shorter critical path. |
| Guarantee offered | None across files; best-effort within one file. | Best-effort across all fields, but a far narrower race window than multi-file I/O. |

Neither approach is truly atomic. The syscall wins by shrinking the observation
window, not by eliminating races.

### Latency

Reading the same seven values from `/proc` requires at minimum:

```
open("/proc/meminfo")  read()  close()   — MemTotal, MemFree, Buffers
open("/proc/stat")     read()  close()   — ctxt (context switches)
open("/proc/uptime")   read()  close()   — uptime
```

That is nine syscalls, each with VFS entry, proc `->show()` string formatting, a
`seq_file` buffer fill, a `copy_to_user` of the formatted text, and a `fput`. The
userspace caller must then parse ASCII strings (locale-sensitive, fragile).

`sys_kstats` is one syscall: one kernel entry, a handful of kernel function calls,
one `copy_to_user` of a typed struct. No string formatting or parsing at either
end. The latency advantage grows with polling frequency and the number of monitored
processes.

### Maintainability

The `/proc` interface has a significant advantage here:

- **No ABI freeze.** Adding a new field to `/proc/meminfo` is a one-line change
  to the `->show()` callback. No struct layout changes, no versioning required.
  Existing scripts that parse only the fields they need continue to work.

- **Human-readable and universally accessible.** `cat /proc/meminfo` works from
  any shell, in any container, without special tooling.

- **Decoupled from kernel build.** `/proc` files are exercised by every Linux user;
  a custom syscall is only tested by code that explicitly calls it.

The syscall approach pays a maintainability tax:

- **ABI is frozen at the first release.** Adding a field to `struct kstats` changes
  the struct size, breaking existing binaries unless a `size` parameter and
  compatibility logic are added.

- **Requires a dedicated header and userspace library.** Callers cannot use the
  syscall from a shell one-liner; they need to `#include "kstats.h"` and call
  `syscall(470, &ks)`.

- **Harder to introspect.** Debugging requires a C program; `/proc` can be read
  with `cat`.

### Summary

| Criterion | `/proc` files | `sys_kstats` syscall |
|---|---|---|
| Atomicity (cross-field) | Weakest — separate files, separate time points | Better — single kernel execution window |
| Latency | Higher — N syscalls + string I/O | Lower — one syscall, typed copy |
| Maintainability | Easier — no ABI freeze, human-readable | Harder — struct layout frozen, needs versioning |
| Appropriate when | One or two fields needed; shell access; rarely polled | Multiple fields needed together; high-frequency polling; typed API preferred |
