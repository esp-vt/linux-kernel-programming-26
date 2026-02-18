# KPTI Investigation

Part 5, Task 5.3 — P1: System Calls: From Theory to Practice
CS 5264/4224 & ECE 5414/4414 — Linux Kernel Programming, Spring 2026

---

## 1. Meltdown vulnerability status

```
$ cat /sys/devices/system/cpu/vulnerabilities/meltdown
Not affected
```

**What this means.**  The running CPU (as seen by the guest VM) reports that it is
not susceptible to Meltdown (CVE-2017-5754).  On modern KVM guests this is common
for two reasons:

* The host may expose a CPU model whose microcode already raises an architectural
  fault *before* the speculative load can retire (e.g. hardware-level fixes in Ice
  Lake / Zen 2 and later).
* The hypervisor may set the CPUID leaf that explicitly marks the CPU as
  unaffected, either because the host CPU is genuinely immune or because nested
  KPTI makes it moot.

Despite the "Not affected" report, the kernel can still be booted with `pti=on`
(forced KPTI), and the protection remains fully functional as shown below.

---

## 2. kpti_test results

### With PTI enabled (default / `pti=on`)

```
$ ./kpti_test
=== KPTI Verification Test ===

  Probe 0xffffffff81000000 (Kernel text start (_text)):
    -> FAULT (KPTI working: kernel memory protected)

  Probe 0xffffffff82000000 (Kernel text (higher)):
    -> FAULT (KPTI working: kernel memory protected)

  Probe 0xffff888000000000 (Direct map (page_offset_base)):
    -> FAULT (KPTI working: kernel memory protected)

Summary
-------
  Total probes:  3
  Faults:        3
  KPTI status:   ENABLED (all kernel addresses are protected)
```

All three probes fault.  The user-mode page tables do not contain any mapping for
kernel virtual addresses; each access generates a SIGSEGV before any data is
returned to userspace.

### With `nopti` (KPTI disabled)

Even with `nopti`, all three probes still fault, and `kpti_test` reports the same
result.  This is expected and does **not** mean KPTI was still active; it means the
hardware page-fault mechanism provides a separate layer of protection:

1. Without KPTI the kernel page-table entries for those addresses still have the
   **Supervisor (U/S = 0)** bit set.  A load executed at CPL 3 (user mode) against
   a supervisor page immediately raises a #PF.  The CPU enforces this
   architecturally, before any data leaves the TLB or cache hierarchy.
2. Meltdown does not bypass that architectural fault — it exploits the
   *speculative* window between when the out-of-order execution unit begins the
   load and when the fault is committed.  During that window the value may have
   been forwarded to a dependent instruction (e.g. a cache-timing gadget).  The
   kpti_test program cannot observe this window because it uses a signal handler,
   not a timing side-channel.

**In short:** `kpti_test` always faults regardless of `nopti` because both KPTI
and the supervisor bit independently block ordinary userspace reads.  The
difference that KPTI makes is that with `nopti` the kernel *is* mapped in the
user page tables (with U/S=0), so speculative microarchitectural leakage is
possible.  With KPTI, the kernel is not mapped at all in the user page tables,
eliminating even the speculative window.

---

## 3. KPTI performance overhead (sys_null benchmark)

| Configuration | `sys_null` avg (ns) |
|---|---|
| PTI enabled (`pti=on`) | 218.9 |
| PTI disabled (`nopti`) | 84.1 |
| **Overhead** | **134.8 ns (+160%)** |

KPTI increases `sys_null` latency by **134.8 ns**, a **160 % slowdown** relative
to the no-PTI baseline.

**Why KPTI adds overhead to every syscall.**  On every kernel entry/exit, the CPU
must switch CR3 — the register that points to the top-level page table:

```
entry_SYSCALL_64:
    swapgs
    mov  %rsp, PER_CPU_VAR(cpu_tss_rw + TSS_sp2)   ; save user RSP
    SWITCH_TO_KERNEL_CR3 scratch_reg=%rsp            ; ← CR3 write #1
    mov  PER_CPU_VAR(cpu_current_top_of_stack), %rsp ; kernel stack

    ... do_syscall_64 ...

syscall_return_via_sysret:
    SWITCH_TO_USER_CR3_STACK scratch_reg=%rdi        ; ← CR3 write #2
    sysretq
```

Each `mov %cr3` serializes the pipeline and, unless PCID is available, also
performs a **full TLB flush** — discarding all cached address translations.
The TLB must then be rebuilt from scratch as the application continues executing,
causing additional cache misses on the first access to each page.  Even with PCID
(Process Context Identifiers, which tag TLB entries so they can be preserved
across CR3 switches), the CR3 write itself is a serialising instruction with
~20–40 cycle latency, and the TLB warm-up cost for the kernel's short-lived
mapping still applies.

---

## 4. SWITCH_TO_KERNEL_CR3: source and explanation

**Location:** `arch/x86/entry/calling.h`, lines 173–179; invoked at
`arch/x86/entry/entry_64.S:94` (the very first lines of `entry_SYSCALL_64`).

```asm
/* calling.h */
.macro SWITCH_TO_KERNEL_CR3 scratch_reg:req
    ALTERNATIVE "jmp .Lend_\@", "", X86_FEATURE_PTI   /* (1) no-op if PTI off */
    mov    %cr3, \scratch_reg                           /* (2) read current CR3 */
    ADJUST_KERNEL_CR3 \scratch_reg                      /* (3) clear PTI bits   */
    mov    \scratch_reg, %cr3                           /* (4) write kernel CR3 */
.Lend_\@:
.endm

.macro ADJUST_KERNEL_CR3 reg:req
    ALTERNATIVE "", "SET_NOFLUSH_BIT \reg", X86_FEATURE_PCID
    andq    $(~PTI_USER_PGTABLE_AND_PCID_MASK), \reg   /* clear bit 12 + PCID  */
.endm
```

**Step-by-step explanation:**

1. **Runtime no-op when PTI is off.**  The `ALTERNATIVE` patching mechanism
   replaces the three instructions with a single `jmp .Lend` at boot time if
   `X86_FEATURE_PTI` is not set, so there is zero overhead when KPTI is
   disabled.

2. **Read CR3.**  The current value of CR3 — which points to the *user* page
   tables while the process was running in userspace — is loaded into the
   scratch register (temporarily using `%rsp` since the kernel stack is not yet
   live at this point in `entry_SYSCALL_64`).

3. **Clear the PTI selector bits (`ADJUST_KERNEL_CR3`).**  The KPTI
   implementation stores *both* the user PGD and the kernel PGD inside the same
   4 KB-aligned structure.  Bit 12 (`PTI_USER_PGTABLE_BIT`) of CR3 selects which
   half is active: bit set → user tables, bit clear → kernel tables.  The `andq`
   mask clears this bit (and the user PCID bits), making CR3 point at the kernel
   page-table root.  If the CPU supports PCID, `SET_NOFLUSH_BIT` additionally
   sets bit 63 of CR3 to suppress the TLB flush.

4. **Write CR3.**  The modified value is written back, completing the switch.
   From this point on, kernel virtual addresses are accessible and user addresses
   remain mapped (the kernel PGD also maps userspace to allow copy_to/from_user).

**Why the kernel needs separate page tables.**  Without KPTI, a single set of
page tables is active at all times, mapping both user and kernel memory.  The
kernel pages are protected only by the Supervisor (U/S) bit — ordinary userspace
loads fault immediately.  However, Meltdown shows that the *speculative* pipeline
can transiently read kernel data before the fault is raised, leaking it through a
cache side-channel.  By ensuring that kernel pages are simply *not present* in the
user-mode page table (only a minimal trampoline is kept), KPTI makes it impossible
for any speculative execution path in user mode to reach kernel data, because there
are no TLB entries or page-walk results pointing to it.

---

## 5. Meltdown and how KPTI prevents it

Meltdown (CVE-2017-5754) exploits the fact that Intel and some other CPUs begin
executing instructions *speculatively* before enforcing access-control checks.
When userspace code loads from a kernel virtual address, the CPU raises a page
fault — but not before the out-of-order execution unit has already fetched the
kernel byte into a register and used it as an index into a user-accessible cache
array.  The fault is then suppressed by a signal handler, but the cache state
persists; the attacker measures access times to the array to recover the secret
byte without ever seeing the fault-inducing value directly.

KPTI prevents this by removing all kernel mappings from the page tables that are
active while the CPU is in user mode.  With no TLB entry and no page-table walk
path to kernel memory, the speculative load can never obtain a kernel virtual
address translation in the first place — the CPU faults at the page-walk stage
before any data is forwarded to dependent instructions.  There is simply nothing
to leak, regardless of how many speculative instructions are in flight.
