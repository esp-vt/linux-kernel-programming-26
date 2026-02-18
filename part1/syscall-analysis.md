# Syscall Analysis

## Task 1.1: Tracing with strace

### 1. `strace -c ls /tmp` — Summary Statistics

```
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 26.40    0.000428          13        31           mmap
 24.37    0.000395          11        34        12 openat
 10.61    0.000172           7        23           fstat
 10.24    0.000166           6        24           close
  5.37    0.000087          43         2           getdents64
  4.57    0.000074          14         5           mprotect
  4.50    0.000073          10         7           read
  2.41    0.000039           6         6           write
  ...
100.00    0.001621          10       153        16 total
```

**Top 3 most frequent syscalls (by call count):**

1. **`openat` (34 calls, 12 errors):** `ls` is dynamically linked, so the runtime
   linker opens every shared library it needs (libc, libselinux, etc.) plus a long
   list of locale-related files for internationalised output. Many paths are tried
   first under `C.UTF-8` and fail with `ENOENT`, then retried under `C.utf8`,
   doubling the attempt count. The actual directory open and the shared-library
   cache also contribute.

2. **`close` (24 calls):** Every file descriptor opened by `openat` must be closed.
   The close count shadows the successful opens.

3. **`fstat` (23 calls):** After the linker opens each library or locale file it
   calls `fstat` to inspect the size and type before mapping or reading it. For the
   directory listing itself, `ls` uses `statx`/`fstat` on each entry to obtain
   metadata (permissions, timestamps) before formatting output.

> Note: `mmap` consumes the most wall-clock time (26 %) despite fewer calls because
> mapping shared libraries involves TLB shootdowns and page-table updates, making each
> call more expensive than a simple open or stat.

---

### 2. `strace -e trace=openat,read,write,close cat /etc/passwd` — File I/O

```
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
close(3)
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF...", 832) = 832
close(3)
...  [many locale file opens/closes] ...
openat(AT_FDCWD, "/etc/passwd", O_RDONLY) = 3
read(3, "root:x:0:0:root:/root:/bin/bash\n...", 131072) = 1947
write(1, "root:x:0:0:root:/root:/bin/bash\n...", 1947) = 1947
read(3, "", 131072) = 0
close(3)
close(1)
close(2)
```

**Top 3 most frequent syscalls (by call count):**

1. **`openat` (~31 calls, ~10 errors):** Before `cat` reads a single byte of
   `/etc/passwd`, the dynamic linker opens the shared-library cache, libc, and a
   cascade of locale-database files (`locale-archive`, `gconv-modules.cache`, and
   one file per `LC_*` category). Each locale category is first tried under the path
   `C.UTF-8` (failing with `ENOENT`) and then under `C.utf8`, roughly doubling the
   number of `openat` calls. The actual `/etc/passwd` open is the last in the
   sequence.

2. **`close` (~21 calls):** Mirrors `openat` — every successfully opened file
   descriptor is closed, including stdout (fd 1) and stderr (fd 2) on exit.

3. **`read` (5 calls):** Reads the libc ELF header (832 bytes), the locale-alias
   file in two chunks (2996 bytes + 0-byte EOF), then `/etc/passwd` in a single
   131072-byte buffer (returning 1947 bytes) followed by a second read returning 0
   to detect EOF. `cat` intentionally uses a large buffer so the entire file is
   transferred in one call, which is why `read` appears far less often than `openat`
   despite being `cat`'s core operation.

> Note: `write` appears only once because the whole file fit in the single read
> buffer and was flushed to stdout in one shot.

---

### 3. `strace -f -e trace=clone,clone3,execve,wait4 bash -c "ls > /dev/null"` — Process Creation

```
execve("/usr/bin/bash", ["bash", "-c", "ls > /dev/null"], ...) = 0
clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD,
      child_tidptr=...) = 1893
wait4(-1, <unfinished ...>
[pid 1893] execve("/usr/bin/ls", ["ls"], ...) = 0
[pid 1893] +++ exited with 0 +++
<... wait4 resumed> [{WIFEXITED(s) && WEXITSTATUS(s) == 0}], 0, NULL) = 1893
--- SIGCHLD {...} ---
wait4(-1, ..., WNOHANG, NULL) = -1 ECHILD
+++ exited with 0 +++
```

**Top 3 most frequent syscalls (by call count, within the filtered set):**

1. **`execve` (2 calls):** First, the shell itself is loaded (`bash -c ...`).
   Second, the child replaces its own image with `ls` after the fork. `execve` is
   called once per program image that gets loaded.

2. **`wait4` (2 calls):** Bash calls `wait4` once blocking (suspending until `ls`
   exits) and once more with `WNOHANG` to confirm no zombie children remain. The
   `SIGCHLD` signal arrives between the two calls.

3. **`clone` (1 call):** Bash forks exactly one child. The redirect `> /dev/null`
   forces this fork: bash must set up the child's stdout to `/dev/null` before
   calling `execve`, and that requires the child to be a separate process. Without
   the redirect, bash would exec directly without forking.

---

### 4. `strace -T -e trace=network curl -s http://example.com > /dev/null` — Network I/O

```
socket(AF_UNIX, ...) = 3  <0.000024>    <- nscd probe #1
connect(3, {sun_path="/var/run/nscd/socket"}) = -1 ENOENT  <0.000048>
socket(AF_UNIX, ...) = 3  <0.000009>    <- nscd probe #2
connect(3, {sun_path="/var/run/nscd/socket"}) = -1 ENOENT  <0.000010>
socket(AF_INET6, SOCK_DGRAM, ...) = 5  <0.000021>   <- IPv6 probe
socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) = 5  <0.000023>  <- real TCP socket
setsockopt(5, SOL_TCP, TCP_NODELAY, [1], 4)  <0.000015>
setsockopt(5, SOL_SOCKET, SO_KEEPALIVE, [1], 4)  <0.000008>
setsockopt(5, SOL_TCP, TCP_KEEPIDLE, [60], 4)  <0.000008>
setsockopt(5, SOL_TCP, TCP_KEEPINTVL, [60], 4)  <0.000008>
connect(5, {sin_addr="104.18.27.120"}, 16) = -1 EINPROGRESS  <0.000056>
getsockname(5, ...) = 0  <0.000008>
getsockopt(5, SOL_SOCKET, SO_ERROR, [0], [4]) = 0  <0.000008>
getsockname(5, ...) = 0  <0.000008>
getpeername(5, ...) = 0  <0.000008>
getsockname(5, ...) = 0  <0.000007>
getpeername(5, ...) = 0  <0.000012>
getsockname(5, ...) = 0  <0.000008>
sendto(5, "GET / HTTP/1.1\r\n...", 74, ...) = 74  <0.000042>
recvfrom(5, "HTTP/1.1 200 OK\r\n...", 102400, ...) = 843  <0.000075>
```

**Top 3 most frequent syscalls (by call count, within the network-filtered set):**

1. **`socket` (4 calls):** curl first probes the Name Service Cache Daemon via two
   `AF_UNIX` sockets (both fail immediately — nscd is not running). It then creates
   an `AF_INET6` datagram socket to test IPv6 reachability, and finally the actual
   `AF_INET` TCP socket for the HTTP connection.

2. **`setsockopt` (4 calls):** curl tunes the TCP connection: `TCP_NODELAY` disables
   Nagle's algorithm for lower latency, `SO_KEEPALIVE` enables keep-alives, and
   `TCP_KEEPIDLE`/`TCP_KEEPINTVL` set the keep-alive timers.

3. **`getsockname` (4 calls) / `connect` (3 calls):** curl queries the local address
   multiple times for its connection-info logging. The three `connect` calls
   correspond to the two failed Unix-socket attempts and the successful (async)
   TCP connect that returned `EINPROGRESS`.

---

## Task 1.2: Kernel Call Tree with ftrace

### Key Subsystems Visible in the Call Tree

#### VFS Layer

The Virtual File System layer provides a uniform file-operation interface across all
filesystems. Every `read()` and `write()` syscall passes through it before reaching
filesystem-specific code. Visible in the trace:

- **`vfs_read()` / `vfs_write()`** — top-level VFS entry points; called from
  `ksys_read()` / `ksys_write()` which are invoked by the architecture-specific
  syscall handler (`__x64_sys_read`, etc.)
- **`rw_verify_area()`** — validates the file offset and length before I/O,
  and triggers the LSM permission check (see Security below)
- **`vfs_fstatat()` / `vfs_statx()`** — VFS entry for attribute queries

Example from the trace (CPU 26, ~line 1702):

```
__x64_sys_read() {
  ksys_read() {
    fdget_pos();            <- resolve fd to struct file*
    vfs_read() {
      rw_verify_area() {
        security_file_permission() { ... }   <- LSM hook
      }
      ext4_file_read_iter() {               <- filesystem dispatch
        filemap_read() { ... }              <- page cache
      }
      __fsnotify_parent();                  <- inotify/fanotify
    }
  }
}
```

#### Security Layer

After `rw_verify_area()` delegates to `security_file_permission()`, the Linux
Security Module framework dispatches to the active LSM. On this kernel AppArmor is
loaded:

```
security_file_permission()
  -> apparmor_file_permission()
       -> common_file_perm()
            -> aa_file_perm()
                 -> __rcu_read_lock()    <- protect policy lookup
                 -> __rcu_read_unlock()
```

AppArmor uses an RCU read-side lock to consult the file-permission policy without
blocking concurrent policy writers. For `stat`-family syscalls the equivalent hook
is `security_inode_getattr()` -> `apparmor_inode_getattr()` -> `common_perm()`.

#### Filesystem Layer (ext4 + Page Cache)

Below the VFS, ext4-specific functions handle the actual I/O:

- **`ext4_file_read_iter()`** — ext4's implementation of `read_iter`, wraps
  `generic_file_read_iter()`
- **`filemap_read()`** — generic page-cache read loop; checks whether the required
  pages are already cached and copies data to userspace. If pages are absent it
  would submit block I/O (not visible here, meaning pages were already cached)
- **`filemap_get_pages()` / `filemap_get_read_batch()`** — locate folios in the
  page cache; the RCU pair inside `filemap_get_read_batch()` serialises against
  page-cache tree modifications
- **`touch_atime()`** — updates the file's access timestamp after a successful read
- **`ext4_file_getattr()`** / **`generic_fillattr()`** — populate the stat
  structure from the inode for `fstat`/`statx` calls

### Why `do_syscall_64` and `entry_SYSCALL_64` Are Not Visible

These two functions are annotated with `noinstr` ("no instrumentation") in the
kernel source. The `noinstr` attribute instructs the compiler to suppress all
instrumentation probes — including the `__fentry__` hooks that ftrace's
function_graph tracer inserts at the entry of every traced function.

The annotation exists because these functions run inside a narrow, hazardous window:

1. **CPU mode transition:** `entry_SYSCALL_64` executes immediately after the
   `SYSCALL` instruction switches the CPU from ring 3 to ring 0. At this point the
   kernel stack is being set up, RCU is not yet watching the CPU, and under KPTI
   the active page table still maps only the entry trampoline — not the full kernel.
   Calling into the ftrace infrastructure here would dereference unmapped memory or
   corrupt not-yet-saved user registers.

2. **KPTI page-table isolation:** With Kernel Page Table Isolation enabled, the
   entry stub runs with a minimal page table (`entry_SYSCALL_64` lives in a special
   trampoline mapping). Ftrace probes need to reach ftrace data structures that
   exist only in the full kernel page tables; accessing them from the entry stub
   would cause an immediate page fault.

3. **RCU and preemption state:** Before `do_syscall_64` calls `rcu_user_exit()` and
   re-enables interrupts, RCU is in an extended-quiescent state. Instrumentation
   code that internally uses RCU or spinlocks would behave incorrectly in this
   context.

The practical consequence is that function_graph tracing only becomes active once
the kernel has fully initialised the CPU context for kernel execution, which is why
the first function visible in the trace is already a specific syscall handler
(e.g., `__x64_sys_read`) rather than the entry path.

---

## Written Questions

### 1. ECF Classification

| Event | Classification | Reasoning |
|---|---|---|
| A program calls `read()` to get data from a file | **Trap** | The process voluntarily executes the `syscall` instruction to request a kernel service. The transition is synchronous and intentional — the process expects to resume after the call returns. |
| A timer fires to preempt a running process | **Interrupt** | The timer chip raises an asynchronous hardware interrupt (IRQ) unrelated to the instruction the CPU is currently executing. The kernel's interrupt handler saves state and may schedule a different process. |
| A program accesses an unmapped memory address | **Fault** | The faulting memory-reference instruction itself triggers a synchronous CPU exception (page fault, vector 14). The kernel may recover (demand paging, copy-on-write) or deliver `SIGSEGV` if the address is truly unmapped. |
| A hardware error detected by the machine check handler | **Abort** | An uncorrectable hardware fault (e.g., ECC memory error, CPU internal error) triggers Machine Check Exception (MCE, vector 18). The system state is deemed unreliable; the process or machine cannot safely continue and recovery is generally not possible. |

---

### 2. Register Convention for x86-64 `syscall`

| Register | Purpose |
|---|---|
| `RAX` | **Syscall number** on entry; **return value** on exit (negative errno on error) |
| `RDI, RSI, RDX, R10, R8, R9` | **Syscall arguments**, up to 6, in this order. `R10` is used instead of `RCX` (the 4th argument slot in the C ABI) because the `SYSCALL` instruction clobbers `RCX`. |
| `RCX` (after `syscall`) | **Saved user-space `RIP`** — the `SYSCALL` instruction writes the return address here so `SYSRET` can restore instruction flow to userspace |
| `R11` (after `syscall`) | **Saved user-space `RFLAGS`** — the `SYSCALL` instruction copies the flags register here and then masks it with `IA32_FMASK` |

---

### 3. Design Philosophy: `fork`/`clone` + `execve` vs. a Single "spawn"

The trace for `bash -c "ls > /dev/null"` shows:

```
execve("/usr/bin/bash", ...)          <- shell loads
clone(CLONE_CHILD_SETTID|SIGCHLD)     <- fork child
  [child] dup2(open("/dev/null"), 1)  <- redirect stdout (not traced here)
  [child] execve("/usr/bin/ls", ...)  <- replace image
wait4(...)                            <- bash reaps child
```

The **window between `clone` and `execve`** is the key. After forking, the child is
a copy of bash with the same open file descriptors. Before calling `execve`, bash
uses that window to reconfigure the child's environment — in this case redirecting
stdout to `/dev/null` with `dup2`. Once `execve` fires, `ls` inherits the
already-configured file descriptors and never needs to know about the redirect.

**Flexibility the two-step design provides:**

| Setup step | Done between fork and exec |
|---|---|
| I/O redirection (`> file`, `2>&1`) | `open()` + `dup2()` in child |
| Pipelines (`cmd1 \| cmd2`) | `pipe()` + `dup2()` on read/write ends |
| Signal mask / dispositions | `sigprocmask()`, `signal(SIG_DFL)` in child |
| Working directory | `chdir()` in child |
| Resource limits | `setrlimit()` in child |
| Tracing / debugging | `ptrace(PTRACE_TRACEME)` in child |
| Credentials | `setuid()` / `setgid()` in child |

A hypothetical single `spawn(program, args, redirects, ...)` syscall would need to
parameterise all of these, producing a complex, ever-growing API. Unix's philosophy
of small, composable primitives keeps each syscall simple while allowing arbitrary
combinations in the inter-fork-exec window.

> Note on the bash optimisation: Without the redirect, bash detects that it has
> nothing to set up in the child and skips the fork entirely, calling `execve`
> directly in the current process. The redirect `> /dev/null` defeats this
> optimisation because bash must wire up the child's stdout without affecting its own
> stdout — which requires a separate process.

---

### 4. Overhead: Fast vs. Slow Syscalls

From the `-T`-annotated curl trace (network syscalls only):

**Fastest observed:** `getsockname(5, ...) = 0 <0.000007>` — **7 µs**

**Slowest observed:** `recvfrom(5, "HTTP/1.1 200 OK\r\n...", 102400, ...) = 843 <0.000075>` — **75 µs**

> Note: Because the trace was filtered to `-e trace=network`, only network syscalls
> appear and none fall below ~7 µs in this dataset. In a complete unfiltered trace,
> pure-computation syscalls such as `getpid()` or `getuid()` routinely complete in
> under 1 µs (just returning a value cached in the kernel's per-process data
> structure), while blocking network calls can reach milliseconds depending on
> round-trip time. The ~10x difference visible here between `getsockname` and
> `recvfrom` illustrates the same principle.

**Why the difference is so large:**

`getsockname` is entirely CPU-bound. It resolves the file descriptor to a
`struct socket`, reads ~16 bytes of the already-populated local address, and copies
them to userspace. No hardware is consulted; the data is hot in L1/L2 cache. The
latency floor is the `syscall`/`sysret` instruction pair itself plus the inevitable
TLB/cache effects of crossing the user/kernel boundary (typically a few hundred
nanoseconds to a few microseconds on a modern CPU).

`recvfrom`, by contrast, must wait for the TCP receive buffer to contain data. Even
in this fast run the call involved:

1. Blocking (or spinning) until the NIC DMA'd the HTTP response into the socket
   receive queue
2. The TCP stack validating checksums, advancing sequence numbers, and ACKing the
   segment
3. Copying bytes from the receive buffer into userspace

In general, the difference between a sub-microsecond syscall and a multi-hundred-
microsecond (or millisecond) syscall comes down to whether the work can be satisfied
entirely from CPU registers and cache (**synchronous, compute-bound**) or whether it
must wait on hardware — a disk seek, a network round-trip, or another process —
before it can return (**I/O-bound, blocking**).
