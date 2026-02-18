# Part 2: GDB / MSR Analysis Answers

## MSR_LSTAR Verification

Running `sudo ./check_msr.sh` gives:

```
=== MSR_LSTAR Verification ===

MSR_LSTAR (0xC0000082) = 0xffffffff81000080

Looking up entry_SYSCALL_64 in symbol tables...
  /proc/kallsyms:  ffffffff81000080 T entry_SYSCALL_64
  System.map:      ffffffff81000080 T entry_SYSCALL_64
```

**MSR_LSTAR = 0xffffffff81000080 = address of `entry_SYSCALL_64`.**
They match exactly: when user code executes `syscall`, the CPU loads RIP from MSR_LSTAR,
jumping directly to `entry_SYSCALL_64` in `arch/x86/entry/entry_64.S`.

---

## `pt_regs` Stack Layout at `do_syscall_64`

The stack is built in two stages:

**Stage 1 — `entry_SYSCALL_64` (entry_64.S:101-107):**
```asm
pushq  $__USER_DS               /* pt_regs->ss     */
pushq  PER_CPU_VAR(TSS_sp2)    /* pt_regs->sp     */
pushq  %r11                     /* pt_regs->flags  (RFLAGS saved by SYSCALL into R11) */
pushq  $__USER_CS               /* pt_regs->cs     */
pushq  %rcx                     /* pt_regs->ip     (return RIP saved by SYSCALL into RCX) */
pushq  %rax                     /* pt_regs->orig_ax (syscall number) */
```

**Stage 2 — `PUSH_REGS` macro (calling.h:76-91):**
```asm
pushq  %rdi    /* pt_regs->di  */
pushq  %rsi    /* pt_regs->si  */
pushq  %rdx    /* pt_regs->dx  */
pushq  %rcx    /* pt_regs->cx  */
pushq  $-ENOSYS/* pt_regs->ax  (default return value, overwritten later) */
pushq  %r8     /* pt_regs->r8  */
pushq  %r9     /* pt_regs->r9  */
pushq  %r10    /* pt_regs->r10 */
pushq  %r11    /* pt_regs->r11 */
pushq  %rbx    /* pt_regs->bx  */
pushq  %rbp    /* pt_regs->bp  */
pushq  %r12    /* pt_regs->r12 */
pushq  %r13    /* pt_regs->r13 */
pushq  %r14    /* pt_regs->r14 */
pushq  %r15    /* pt_regs->r15 */
```

After these pushes, `movq %rsp, %rdi` passes the pointer to `do_syscall_64`.

```
     Address        Field             Notes
     ──────────────────────────────────────────────────────────────
                  ┌─────────────┐
  high addr       │     ss      │  ← FIRST pushed; pt_regs->ss
  (bottom)        ├─────────────┤
                  │     sp      │  pt_regs->sp  (saved user RSP)
                  ├─────────────┤
                  │    flags    │  pt_regs->flags  (= user RFLAGS via R11)
                  ├─────────────┤
                  │     cs      │  pt_regs->cs
                  ├─────────────┤
                  │     ip      │  pt_regs->ip  (= user RIP via RCX)
                  ├─────────────┤
                  │   orig_ax   │  pt_regs->orig_ax  (syscall number)
                  ├─────────────┤
                  │     di      │  pt_regs->di
                  ├─────────────┤
                  │     si      │  pt_regs->si
                  ├─────────────┤
                  │     dx      │  pt_regs->dx
                  ├─────────────┤
                  │     cx      │  pt_regs->cx
                  ├─────────────┤
                  │     ax      │  pt_regs->ax  (initialised to -ENOSYS)
                  ├─────────────┤
                  │     r8      │  pt_regs->r8
                  ├─────────────┤
                  │     r9      │  pt_regs->r9
                  ├─────────────┤
                  │    r10      │  pt_regs->r10
                  ├─────────────┤
                  │    r11      │  pt_regs->r11
                  ├─────────────┤
                  │     bx      │  pt_regs->bx
                  ├─────────────┤
                  │     bp      │  pt_regs->bp
                  ├─────────────┤
                  │    r12      │  pt_regs->r12
                  ├─────────────┤
                  │    r13      │  pt_regs->r13
                  ├─────────────┤
                  │    r14      │  pt_regs->r14
                  ├─────────────┤
  low addr        │    r15      │  ← LAST pushed; RSP points here
  (top)           └─────────────┘
```

**First pushed (bottom / highest address):** `ss`
**Last pushed (top / lowest address, RSP):** `r15`

---

## Question Answers

### 1. swapgs — Why is it the very first instruction?

When `syscall` fires, the CPU is still running with the **user GS base** in the hidden GS
descriptor (the value in `MSR_GS_BASE`).  The kernel must immediately replace it with the
**kernel GS base** (stored in `MSR_KERNEL_GS_BASE`) before it can safely dereference any
per-CPU variable.

`swapgs` atomically exchanges:

| Slot | Value |
|------|-------|
| `MSR_GS_BASE` (active) | **User GS base** — userspace thread-local storage pointer (e.g. glibc's TLS `%fs`/`%gs` descriptor, or 0) |
| `MSR_KERNEL_GS_BASE` (shadow) | **Kernel GS base** — points to the per-CPU `struct pcpu_hot` / TSS area |

The kernel needs its own GS base because the very next instructions use
`PER_CPU_VAR(cpu_tss_rw + TSS_sp2)` and `PER_CPU_VAR(cpu_current_top_of_stack)`, which are
GS-relative loads.  Executing those with the user GS base in place would dereference
arbitrary userspace memory — an immediate security disaster.  `swapgs` must come first,
before any stack switch or register save, because there is no safe kernel stack yet.

---

### 2. R10 vs RCX — Why is R10 the 4th syscall argument?

The `syscall` instruction **clobbers RCX** as part of its hardware mechanism: it saves the
user-space return address (`RIP + instruction length`) into `RCX` so that `sysretq` can
restore it later.  By the time execution reaches the kernel entry point, `RCX` no longer
holds the 4th function argument — it holds the user return address.

The Linux syscall ABI therefore uses **R10** for the 4th argument (instead of RCX as the
System V AMD64 C calling convention dictates).  A syscall wrapper in libc typically does:

```asm
mov  %rcx, %r10    ; move 4th arg from C convention slot into R10
syscall            ; RCX is now clobbered with return RIP
```

Inside the kernel, `do_syscall_64` sees the 4th argument in `pt_regs->r10`, not
`pt_regs->cx`.  The `PUSH_REGS` macro saves both, but `pt_regs->cx` at entry contains the
user return address, not arg4.

---

### 3. Register Clearing — Why zero registers that were just saved?

After `PUSH_REGS` saves all GPRs onto the kernel stack, `CLEAR_REGS` zeros them in-place
(via `xorl %esi, %esi`, etc.).  This is a **Spectre variant 1 (bounds-check bypass)**
mitigation.

The threat model: an attacker can poison CPU branch predictors to cause the processor to
*speculatively* execute a kernel code gadget with attacker-controlled values in registers —
values that were still live in the registers from userspace.  Even though those speculative
loads are architecturally squashed, they may leave **cache side-channel evidence** that
leaks kernel data (e.g. using the register as an out-of-bounds array index into a kernel
buffer).

By zeroing the registers immediately after saving them, the kernel ensures that **no
user-controlled value is in any GPR** during the subsequent speculative execution window
(IBRS/IBPB aside).  Any gadget that speculatively uses a register will see 0, not an
attacker-crafted pointer or index, making the gadget useless.

The saved values on the stack are safe because the kernel addresses them through a trusted
`struct pt_regs *` pointer and accesses them deliberately, not speculatively through an
attacker-controlled index.

---

### 4. KPTI Trampoline — Separate page tables for userspace return

**The problem — Meltdown (CVE-2017-5754):**
Before KPTI, the full kernel address space was mapped in every process's page tables (with
supervisor-only permissions).  Meltdown showed that speculative execution in userspace
could *read* supervisor pages before the permission check retired, leaking kernel memory
via cache timing.

**KPTI's solution — two CR3 values per CPU:**

| Page table set | What is mapped |
|----------------|---------------|
| *Kernel* CR3 | Full kernel VA + user VA (normal operation while in kernel) |
| *User* CR3   | Only user VA + a tiny **trampoline** region of kernel code/data |

**The trampoline stack** (`TSS_sp0`, a per-CPU page that is present in *both* page-table
sets) is needed because the CR3 switch cannot happen while using the normal kernel stack:
the kernel stack is mapped only in the kernel CR3.  The sequence on return to userspace is:

1. Pop all saved GPRs from the kernel stack (still on kernel CR3).
2. Switch RSP to the **trampoline stack** (`TSS_sp0`) — a page mapped in both CR3s.
3. Copy the IRET frame (SS/RSP/RFLAGS/CS/RIP) to the trampoline stack.
4. Flip CR3 to the *user* page tables (`SWITCH_TO_USER_CR3`).
5. `swapgs`, `iretq` / `sysretq` — now executing from a page that is still mapped.

Without the trampoline stack, after the CR3 switch the CPU's RSP would point into kernel
memory that is *no longer mapped*, causing an immediate fault.  The trampoline provides a
safe landing pad: a small, deliberately shared piece of memory that allows the CPU to
finish the return sequence after the page-table switch.
