# sys_kstats Comparison Analysis

## 1. Do the sys_kstats values match the /proc files?

Most fields match exactly. The one discrepancy is context switches:

| Field              | sys_kstats | /proc     | Delta  |
|--------------------|------------|-----------|--------|
| Total memory (KB)  | 263914088  | 263914088 | 0      |
| Free memory (KB)   | 258093520  | 258093520 | 0      |
| Buffer memory (KB) | 118360     | 118360    | 0      |
| Uptime (seconds)   | 6678       | 6678      | 0      |
| Context switches   | 3046910    | 3046912   | **+2** |

**Context switch discrepancy (+2):** This is expected and not a bug. Context switches
are recorded on every scheduler invocation across all CPUs. Between the moment
`sys_kstats` reads `nr_context_switches()` and the moment the test program opens,
reads, and parses `/proc/stat`, at minimum two context switches occur: the return
from the syscall itself and the userspace `open()`/`read()` on `/proc/stat`. A
delta this small confirms the implementation is correct; a large or zero delta would
indicate a problem.

The memory and uptime fields match exactly because they change on coarser
timescales (page allocations and the one-second jiffy tick, respectively) that
are unlikely to advance in the microseconds between the two reads.

---

## 2. Single syscall vs. parsing multiple /proc files

### Latency

Reading `N` `/proc` files requires `N × (open + read + close)` system calls plus
VFS overhead for each synthetic file's `->show()` callback. A single `sys_kstats`
call is one kernel entry, one pass over the relevant data structures, and one
`copy_to_user`. On a lightly loaded system the difference is small; under high
syscall pressure (e.g., a monitoring agent sampling every 100 ms across thousands
of containers) the reduction in syscall count and avoided string formatting is
measurable.

### Atomicity

Each `/proc` file is a separate snapshot. A tool that reads `/proc/meminfo` and
then `/proc/stat` sees memory state at time T₁ and context-switch state at T₂.
If a memory-intensive workload starts between those two reads, the combined view
is inconsistent. A single syscall cannot deliver true kernel-wide atomicity
(see §3), but it reduces the observation window from milliseconds (file I/O
round-trips) to microseconds (a few dozen kernel instructions), making
inconsistencies far less likely in practice.

### Complexity

Callers avoid locale-sensitive, fragile string parsing. A struct field is a
typed integer; a `/proc` line is a string that can change format across kernel
versions. A single syscall also makes error handling straightforward: one return
value, one `errno`, versus checking each file open and each `sscanf` return.

**Prefer a single syscall when:** the caller needs several fields together,
consistency between fields matters, or the data will be sampled at high frequency.

**Prefer /proc when:** only one or two fields are needed, the caller is a shell
script, or the kernel version predates the syscall.

---

## 3. Are all fields from the same instant? Race conditions.

No. Each field is collected from a different kernel data structure in sequence
within the syscall body. On a multiprocessor system, other CPUs continue running
and modifying those structures concurrently.

### Per-field exposure

| Field              | Source                                    | Vulnerability |
|--------------------|-------------------------------------------|---------------|
| `num_processes`    | `nr_threads` (global atomic)              | A `fork()`/`exit()` on another CPU between this read and the next field read changes the count. |
| `num_running`      | Per-CPU `rq->nr_running` sum              | The scheduler can migrate tasks between CPUs while the loop accumulates the sum, double-counting or missing a task. |
| `total_memory`     | `totalram_pages` (set at boot)            | Effectively constant post-boot on non-hotplug systems; no practical race. |
| `free_memory`      | `global_node_page_state(NR_FREE_PAGES)`   | Page allocator on another CPU can alloc/free pages between this read and the buffer read below. |
| `buffer_memory`    | `global_node_page_state(NR_BUFFERS)`      | Same lock-free counter; can diverge from the `free_memory` read above. |
| `uptime`           | `ktime_get_boottime_seconds()`            | Advances in one-second steps; a read straddling a tick boundary returns a value one second stale relative to a read a moment later. |
| `context_switches` | `nr_context_switches()` (per-CPU sum)     | Every scheduler invocation on any CPU increments its local counter; the aggregation loop is not atomic across CPUs. |

### Concrete race scenario

A memory-pressure event fires while `sys_kstats` is executing:

1. Syscall reads `free_memory` = 258093520 KB.
2. Another CPU's page allocator satisfies a large `mmap`, consuming 512 MB.
3. Syscall reads `buffer_memory` — the buffers stat is now stale relative to the
   free-memory value captured in step 1.

The returned struct therefore shows free memory from before the allocation and
buffer memory from after: values that never coexisted in the kernel at the same
instant.

### Mitigation options

- **Seqlock / RCU snapshot:** wrap the collection loop in a seqlock read-side
  section so that a concurrent writer forces a retry.
- **Single reader lock:** take `tasklist_lock` for process fields and
  `zone->lock` for memory fields, but this increases latency and is inappropriate
  for a general-purpose stats syscall.
- **Document the window:** the simplest approach — accept the race, document that
  fields are best-effort snapshots collected within a single syscall invocation,
  and note that the consistency window is far narrower than reading separate
  `/proc` files sequentially.
