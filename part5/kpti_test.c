/*
 * kpti_test.c - KPTI (Kernel Page Table Isolation) verification
 *
 * Part 5, Task 5.3 of P1: System Calls - From Theory to Practice
 * CS 5264/4224 & ECE 5414/4414 - Linux Kernel Programming, Spring 2026
 *
 * KPTI is a defense against Meltdown (CVE-2017-5754).  When enabled,
 * the kernel maintains TWO sets of page tables per process:
 *
 *   1. Kernel page tables: map both kernel and user memory
 *      (used only while executing in kernel mode)
 *
 *   2. User page tables: map ONLY user memory + a tiny kernel stub
 *      (used while executing in user mode)
 *
 * On every syscall entry, the kernel switches CR3 from the user page
 * tables to the kernel page tables (SWITCH_TO_KERNEL_CR3).  On return,
 * it switches back (SWITCH_TO_USER_CR3).  This means userspace code
 * can never read kernel memory, even speculatively (Meltdown).
 *
 * This program probes kernel addresses from userspace.  With KPTI
 * enabled, every probe should trigger a SIGSEGV (segmentation fault).
 *
 * Build:
 *   make part5
 *   (or: gcc -O2 -Wall -o kpti_test kpti_test.c)
 *
 * Run:
 *   ./kpti_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <stdint.h>

static sigjmp_buf jump_buf;
static volatile int fault_count = 0;

static void segv_handler(int sig)
{
	(void)sig;
	fault_count++;
	siglongjmp(jump_buf, 1);
}

/*
 * Attempt to read a single byte from the given address.
 * Returns 0 if readable (BAD -- KPTI not working).
 * Returns -1 if faulted (GOOD -- kernel memory is protected).
 */
static int probe_address(unsigned long addr)
{
	volatile uint8_t val;

	if (sigsetjmp(jump_buf, 1) == 0) {
		val = *(volatile uint8_t *)addr;
		(void)val;
		return 0;   /* No fault -- KPTI may not be active */
	}
	return -1;          /* Faulted -- KPTI is working */
}

int main(void)
{
	struct {
		unsigned long addr;
		const char *description;
	} probes[] = {
		/*
		 * These are standard x86-64 kernel virtual address ranges.
		 * See Documentation/arch/x86/x86_64/mm.rst in the kernel
		 * source for the full memory map.
		 */
		{ 0xffffffff81000000UL, "Kernel text start (_text)" },
		{ 0xffffffff82000000UL, "Kernel text (higher)" },
		{ 0xffff888000000000UL, "Direct map (page_offset_base)" },
	};
	int num_probes = sizeof(probes) / sizeof(probes[0]);
	int i;

	printf("=== KPTI Verification Test ===\n\n");

	/* Install SIGSEGV handler with SA_NODEFER to allow re-entry */
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = segv_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NODEFER;
	sigaction(SIGSEGV, &sa, NULL);

	/* Probe each kernel address */
	for (i = 0; i < num_probes; i++) {
		int result = probe_address(probes[i].addr);
		printf("  Probe 0x%016lx (%s):\n", probes[i].addr,
		       probes[i].description);
		if (result < 0) {
			printf("    -> FAULT (KPTI working: kernel memory protected)\n\n");
		} else {
			printf("    -> READABLE (WARNING: KPTI may not be active!)\n\n");
		}
	}

	/* Summary */
	printf("Summary\n");
	printf("-------\n");
	printf("  Total probes:  %d\n", num_probes);
	printf("  Faults:        %d\n", fault_count);
	printf("  KPTI status:   %s\n\n",
	       fault_count == num_probes
		   ? "ENABLED (all kernel addresses are protected)"
		   : "WARNING: some probes did not fault!");

	/*
	 * TODO: Write your analysis in kpti-analysis.md:
	 *
	 * 1. How does KPTI work?  (Two sets of page tables, CR3 switching)
	 * 2. What is SWITCH_TO_KERNEL_CR3 in the syscall entry path?
	 * 3. What is the performance cost of KPTI?
	 * 4. How does KPTI prevent Meltdown?
	 */

	return (fault_count == num_probes) ? 0 : 1;
}
