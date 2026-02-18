/*
 * seccomp_filter.c - seccomp BPF filter with allowlist
 *
 * Part 5, Task 5.2 of P1: System Calls - From Theory to Practice
 * CS 5264/4224 & ECE 5414/4414 - Linux Kernel Programming, Spring 2026
 *
 * Unlike strict mode (which only allows 4 syscalls), BPF filters let
 * you define a custom policy.  The filter is a BPF (Berkeley Packet
 * Filter) program that inspects each syscall's number and arguments,
 * then returns an action: ALLOW, ERRNO, KILL, TRAP, TRACE, or LOG.
 *
 * In this program, we:
 *   - ALLOW a specific set of syscalls needed for basic I/O
 *   - ALLOW your custom sys_kstats syscall
 *   - DENY everything else with EPERM (not SIGKILL -- graceful denial)
 *
 * Build:
 *   make part5
 *   (or: gcc -O2 -Wall -o seccomp_filter seccomp_filter.c)
 *
 * Run:
 *   ./seccomp_filter
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <linux/audit.h>
#include <stddef.h>

#include "../part3/kstats.h"   /* __NR_kstats */

/*
 * BPF filter macros.
 *
 * A BPF filter is a sequence of instructions that examine the
 * seccomp_data structure:
 *
 *   struct seccomp_data {
 *       int   nr;          // syscall number
 *       __u32 arch;        // AUDIT_ARCH_*
 *       __u64 instruction_pointer;
 *       __u64 args[6];     // syscall arguments
 *   };
 *
 * Each ALLOW_SYSCALL macro:
 *   1. Compares the loaded syscall number with nr
 *   2. If equal, jumps to the ALLOW return
 *   3. If not, falls through to the next check
 */
#define ALLOW_SYSCALL(nr)                                                 \
	BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, (nr), 0, 1),               \
	BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW)

/*
 * The filter program.
 *
 * TODO: Add any additional syscalls your program needs.
 *       If the program crashes with "Operation not permitted", check
 *       dmesg or strace to find which syscall was blocked, then add
 *       it to this list.
 */
static struct sock_filter filter[] = {
	/* Load the syscall number from seccomp_data.nr */
	BPF_STMT(BPF_LD | BPF_W | BPF_ABS,
		 offsetof(struct seccomp_data, nr)),

	/* Allow essential I/O syscalls */
	ALLOW_SYSCALL(__NR_read),
	ALLOW_SYSCALL(__NR_write),
	ALLOW_SYSCALL(__NR_close),
	ALLOW_SYSCALL(__NR_fstat),
	ALLOW_SYSCALL(__NR_newfstatat),

	/* Allow memory management (needed by libc) */
	ALLOW_SYSCALL(__NR_brk),
	ALLOW_SYSCALL(__NR_mmap),
	ALLOW_SYSCALL(__NR_munmap),
	ALLOW_SYSCALL(__NR_mprotect),

	/* Allow process exit */
	ALLOW_SYSCALL(__NR_exit_group),
	ALLOW_SYSCALL(__NR_exit),

	/* Allow signal handling */
	ALLOW_SYSCALL(__NR_rt_sigaction),
	ALLOW_SYSCALL(__NR_rt_sigreturn),

	/* Allow your custom syscall */
	ALLOW_SYSCALL(__NR_kstats),

	/* TODO: If you implemented sys_procinfo, add it here: */
	/* ALLOW_SYSCALL(__NR_procinfo), */

	/* Default: deny with EPERM (not SIGKILL -- graceful failure) */
	BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
};

static struct sock_fprog prog = {
	.len    = sizeof(filter) / sizeof(filter[0]),
	.filter = filter,
};

int main(void)
{
	struct kstats ks;
	long ret;
	int fd;

	printf("=== seccomp BPF Filter Demo ===\n\n");

	/*
	 * Step 1: Allow ourselves to install seccomp filters.
	 * PR_SET_NO_NEW_PRIVS ensures the filter can't be bypassed
	 * by execve'ing a setuid binary.
	 */
	if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) < 0) {
		perror("prctl(NO_NEW_PRIVS)");
		return 1;
	}

	/* Step 2: Install the BPF filter */
	if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog) < 0) {
		perror("prctl(SECCOMP_MODE_FILTER)");
		return 1;
	}

	printf("[*] seccomp BPF filter installed.\n\n");

	/* --- Test 1: Allowed syscall (sys_kstats) -------------------------- */
	printf("[Test 1] Calling sys_kstats (should succeed)...\n");
	ret = syscall(__NR_kstats, &ks);
	if (ret == 0) {
		printf("  SUCCESS: %lu processes, %lu KB free memory\n\n",
		       ks.nr_processes, ks.free_memory_kb);
	} else {
		printf("  FAILED: %s\n\n", strerror(errno));
	}

	/* --- Test 2: Denied syscall (open) --------------------------------- */
	printf("[Test 2] Calling open() (should be denied with EPERM)...\n");
	fd = open("/etc/passwd", 0);
	if (fd < 0) {
		printf("  DENIED: %s (errno=%d)\n", strerror(errno), errno);
		if (errno == EPERM)
			printf("  CORRECT: Got EPERM as expected (not killed!)\n\n");
		else
			printf("  UNEXPECTED: Expected EPERM (errno=1)\n\n");
	} else {
		printf("  WARNING: open() succeeded -- filter is not working!\n\n");
		close(fd);
	}

	/* --- Test 3: Denied syscall (getpid) ------------------------------- */
	printf("[Test 3] Calling getpid() (should be denied)...\n");

	/*
	 * TODO: Call getpid() or syscall(__NR_getpid) and handle the EPERM
	 * error gracefully.  Unlike strict mode, BPF filters with
	 * SECCOMP_RET_ERRNO don't kill the process -- they just make the
	 * syscall return -1 with the specified errno.
	 */

	printf("\n[*] Process survived!  (BPF filter uses EPERM, not SIGKILL)\n");
	return 0;
}
