/*
 * test_syscall.c - Simple program to trigger a known syscall for GDB analysis
 *
 * Part 2, Task 2.1 of P1: System Calls - From Theory to Practice
 * CS 5264/4224 & ECE 5414/4414 - Linux Kernel Programming, Spring 2026
 *
 * This program invokes getpid() via the raw syscall interface so that
 * the syscall number and arguments are predictable.  Run it inside the
 * Linux VM while GDB is attached to the kernel; set a breakpoint on
 * do_syscall_64 and inspect the pt_regs structure.
 *
 * Expected values when the breakpoint fires:
 *   orig_ax = 39          (__NR_getpid on x86-64)
 *   di      = (unused)    getpid takes no arguments
 *
 * Compile:
 *   gcc -O0 -g -o test_syscall test_syscall.c
 *
 * The -O0 flag prevents the compiler from optimizing away the syscall.
 * The -g flag adds debug symbols for easier GDB inspection.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

int main(void)
{
	long pid;

	printf("About to call getpid() via syscall()...\n");
	printf("Expected syscall number (orig_ax): %d (__NR_getpid)\n",
	       __NR_getpid);

	/*
	 * Use the raw syscall() interface so the syscall number is
	 * placed directly in RAX, with no libc caching.
	 */
	pid = syscall(__NR_getpid);

	printf("getpid() returned: %ld\n", pid);

	/*
	 * TODO (Task 2.1, item 4-5):
	 *
	 * Add additional syscalls with known arguments so you can verify
	 * the argument registers (di, si, dx) in pt_regs.  For example:
	 *
	 *   syscall(__NR_write, STDOUT_FILENO, "hello\n", 6);
	 *
	 * Expected pt_regs for the write() call above:
	 *   orig_ax = 1    (__NR_write)
	 *   di      = 1    (STDOUT_FILENO)
	 *   si      = <address of "hello\n">
	 *   dx      = 6    (count)
	 */

	return 0;
}
