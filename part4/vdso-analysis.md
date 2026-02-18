# Part 4: vDSO Analysis

## System

- **CPU:** Intel Xeon 6960P @ 2.70 GHz
- **Kernel:** 6.18.0esp-LKP26-dirty
- **vvar:** `0x7eee9a2f8000–0x7eee9a2fc000` (16 KiB, 4 pages)
- **vdso:** `0x7eee9a2fe000–0x7eee9a300000` (8 KiB, 2 pages)

---

## Exported vDSO Symbols

```
$ objdump -T vdso.so

DYNAMIC SYMBOL TABLE:
0000000000000820 g    DF .text  000002d1  LINUX_2.6  __vdso_gettimeofday
0000000000000820  w   DF .text  000002d1  LINUX_2.6  gettimeofday
0000000000000b30 g    DF .text  000003aa  LINUX_2.6  __vdso_clock_gettime
0000000000000b30  w   DF .text  000003aa  LINUX_2.6  clock_gettime
0000000000000ee0 g    DF .text  00000081  LINUX_2.6  __vdso_clock_getres
0000000000000ee0  w   DF .text  00000081  LINUX_2.6  clock_getres
0000000000000b00 g    DF .text  0000002d  LINUX_2.6  __vdso_time
0000000000000b00  w   DF .text  0000002d  LINUX_2.6  time
0000000000000f70 g    DF .text  0000002c  LINUX_2.6  __vdso_getcpu
0000000000000f70  w   DF .text  0000002c  LINUX_2.6  getcpu
0000000000000fa0 g    DF .text  00000372  LINUX_2.6  __vdso_getrandom
0000000000000fa0  w   DF .text  00000372  LINUX_2.6  getrandom
0000000000001540 g    DF .text  0000009c  LINUX_2.6  __vdso_sgx_enter_enclave
0000000000000000 g    DO *ABS*  00000000  LINUX_2.6  LINUX_2.6
```

Eight exported symbols (plus the ABI version marker), each with both a `__vdso_` prefixed
name and a plain alias:

| Symbol | Size | Purpose |
|--------|------|---------|
| `__vdso_gettimeofday` | 721 B | Wall-clock time (seconds + microseconds) |
| `__vdso_clock_gettime` | 938 B | POSIX clocks (MONOTONIC, REALTIME, …) |
| `__vdso_clock_getres` | 129 B | Clock resolution query |
| `__vdso_time` | 45 B | Coarse seconds-since-epoch (`time_t`) |
| `__vdso_getcpu` | 44 B | Current CPU and NUMA node number |
| `__vdso_getrandom` | 882 B | Cryptographic random bytes (kernel 6.11+) |
| `__vdso_sgx_enter_enclave` | 156 B | SGX enclave entry trampoline |

---

## Q1. How Does the vDSO Read Kernel Time Without a Privilege Transition?

### The vvar page

When the kernel maps the vDSO into a process it also maps a companion **vvar page**
immediately before it (visible above: vvar at `…f8000`, vdso at `…fe000`).  The vvar
page is mapped **read-only** into userspace but is **writable by the kernel** — the
kernel's timekeeping tick updates it in place on every clock interrupt.  It holds a
`struct vdso_data` for each hardware clock:

| Field | What it stores |
|-------|----------------|
| `seq` | Seqcount generation counter (odd = kernel is writing, even = stable) |
| `clock_mode` | Which hardware source: `VDSO_CLOCKMODE_TSC` (1) or HPET (2) |
| `cycle_last` | TSC value captured at last timekeeping update |
| `max_cycles` | Overflow guard for the TSC delta |
| `mult` | Multiplier: TSC cycles → nanoseconds |
| `shift` | Right-shift amount to finish the TSC → ns conversion |
| `basetime[]` | Base wall-clock / monotonic nanoseconds at `cycle_last` |
| `tz_minuteswest`, `tz_dsttime` | Timezone for `gettimeofday` |

### The fast path: TSC + shared memory, no `syscall` instruction

`__vdso_clock_gettime` (representative of all the time functions) works as follows —
confirmed by the disassembly at offsets `0xb30–0xbff`:

```
1. Load seq counter from vvar:
       b69: mov (%r9),%r8d          ; r9 = vvar base (RIP-relative, negative offset)
       b6c: test $0x1,%r8b          ; odd => writer active, spin

2. Read clock_mode from vvar:
       b76: mov 0x4(%r9),%eax
       b7a: cmp $0x1,%eax           ; VDSO_CLOCKMODE_TSC?

3. Capture TSC (serializing):
       b83: rdtscp                  ; reads TSC into rdx:rax, CPU-id into ecx

4. Compute elapsed cycles since last update:
       b9b: sub 0x8(%r9),%rdx       ; rdx -= cycle_last
       baa: cmp 0x10(%r9),%rdx      ; check against max_cycles (overflow guard)

5. Convert cycles → nanoseconds (mult * delta >> shift):
       bb4: mov 0x20(%r9),%eax      ; load mult
       bb8: imul %rdx,%rax
       bb4: add %r11,%rax           ; add per-clock offset (base_time)
       bbf: shr %cl,%rax            ; shift

6. Re-read seq counter and compare:
       bc6: mov (%r9),%edx
       bc9: cmp %edx,%r8d
       bcb: jne b69                 ; mismatch => kernel updated mid-read, retry

7. Store result into struct timespec:
       bec: mov %rcx,(%rsi)         ; tv_sec
       bef: mov %rax,0x8(%rsi)      ; tv_nsec
```

**Key insight:** every memory access (`mov … (%r9)`) reads from the vvar page — a
regular user-mode load from a mapped page.  No `syscall`, no `int 0x80`, no ring
transition.  The CPU never leaves ring 3.

### Concurrency: seqcount lock-free protocol

Because the kernel can update the vvar page mid-read (e.g., during an NMI), the vDSO
uses a **seqcount**: a monotone counter the kernel increments twice per update (odd while
writing, even when done).  If the counter is odd when the vDSO starts reading, it spins
(`pause; jmp`).  After reading, it compares the saved value to the current one; a
mismatch means the kernel wrote a new snapshot during the read, so it retries.  This
gives linearizable reads with zero kernel involvement.

### Fallback to real syscall

For clock IDs the fast path cannot handle (e.g., `CLOCK_REALTIME_COARSE` on some
configurations, or unknown clock IDs), the vDSO falls back to a real `syscall`
instruction (visible at offset `0xc23`: `syscall` with `%eax = 0xe4` = `__NR_clock_gettime`).

### `__vdso_getcpu` — no memory at all

The simplest case: `__vdso_getcpu` issues a single `rdpid %rax` instruction (offset `0xf75`),
which reads the current CPU/NUMA node directly from the `IA32_TSC_AUX` MSR without any
memory access or privilege transition.

---

## Q2. Why Can't All Syscalls Use vDSO?

The vDSO trick works only when **all information the function needs can be placed in a
read-only shared page that the kernel updates in the background**.  Time fits: the kernel
already maintains a single global timekeeping snapshot; userspace just needs to read it.

Two fundamental reasons a syscall cannot use vDSO:

### Reason 1 — The operation mutates kernel state

`write(fd, buf, n)` must:
- Copy bytes into a kernel buffer
- Update the file's `struct file` position counter
- Wake any blocked readers on the file
- Dirty the page cache and schedule writeback

None of this can happen from ring 3 without the kernel's involvement.  There is no
shared page the vDSO could write to on behalf of the kernel — writing to kernel data
structures from userspace is precisely what ring 3 prevents.

Other examples in this category: `open`, `close`, `read`, `mmap`, `fork`, `clone`,
`kill`, `socket`, `connect`, `accept`.  Every syscall that creates, modifies, or destroys
kernel objects (file descriptors, sockets, processes, memory mappings) is in this category.

### Reason 2 — The result depends on who is asking or on opaque kernel state

`getpid()` returns the PID of the calling process.  This was actually cached by glibc
before v2.25 (a userspace hack, not a vDSO), but glibc removed the cache because of
correctness bugs with `clone()`/`unshare()`.  A vDSO cannot do it correctly either:
the vDSO page is **shared across all processes** (it is the same physical page mapped
into every address space).  It cannot contain per-process data — the moment the kernel
needs per-caller state (PID, UID, open FD table, current working directory) the shared
page model breaks down.

Similarly, `epoll_wait` and `futex` block until an event occurs.  They need the kernel's
scheduler to park the thread and wake it later.  A polling loop in a shared vDSO page
would waste CPU and still could not receive an interrupt-driven wakeup without a kernel
trap.

**Summary:** vDSO is limited to read-only queries against a small, infrequently-written
global snapshot.  Any syscall that (a) changes kernel state, (b) depends on per-process
state, or (c) must block waiting for an event, cannot use vDSO.

---

## Q3. Why Is vDSO Mapped at a Randomized Address (ASLR)?

The vDSO is ELF code mapped into every process.  Without ASLR it would appear at the
same virtual address in every process on the system (e.g., `0x7ffff7ffd000` always).

### Attack vector without ASLR: return-to-vDSO / ROP

The vDSO contains several useful instruction sequences for attackers:

- A real `syscall` instruction (at a known offset in `__vdso_gettimeofday` and
  `__vdso_clock_gettime`, used for the fallback path).
- `ret` instructions scattered throughout every function.
- `rdtscp`, `rdpid`, arithmetic gadgets.

If an attacker controls the stack (via a stack buffer overflow or use-after-free), they
can overwrite the return address to jump into vDSO code.  With a fixed address they can:

1. **Execute an arbitrary syscall** — jump to the `syscall` instruction inside the vDSO
   with registers pre-set (`rax = __NR_execve`, `rdi` → `/bin/sh`, …) to spawn a shell
   without needing libc or any other library in the address space.

2. **Build a ROP chain** — chain `ret`-terminated gadgets from the vDSO to construct
   arbitrary computation.  Because the vDSO is present in every process, gadgets from it
   are universally reusable across binaries.

### Why ASLR defeats this

With ASLR the kernel randomizes the base address of the vDSO at each `execve` (and at
each mmap call on Linux ≥ 4.8 for the vDSO).  An attacker who does not already have
a memory disclosure primitive cannot know where the `syscall` instruction or any gadget
lives.  A blind jump into the vDSO region is just as likely to land in the middle of an
instruction, causing a crash, as to find a useful gadget.

ASLR also protects the **vvar page** (mapped adjacent to the vDSO) from similar read
gadgets that could leak kernel data structure addresses.

> Note: ASLR entropy for the vDSO on x86-64 Linux is typically 28 bits (one of the
> highest in the address space layout), making brute-force infeasible even against
> long-running daemons.
