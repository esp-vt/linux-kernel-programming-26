/*
 * test_procinfo.c - Test program for the sys_procinfo system call
 *
 * Part 3, Task 3.3 (Advanced) of P1: System Calls - From Theory to Practice
 * CS 5264/4224 & ECE 5414/4414 - Linux Kernel Programming, Spring 2026
 *
 * Tests sys_procinfo() with various PIDs and error conditions.
 *
 * Compile:
 *   make part3
 *   (or: gcc -O2 -Wall -o test_procinfo test_procinfo.c)
 *
 * Run (inside Linux VM with your custom kernel):
 *   ./test_procinfo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "kstats.h"

static const char *state_name(long state)
{
	switch (state) {
	case 0x0000: return "RUNNING";
	case 0x0001: return "INTERRUPTIBLE";
	case 0x0002: return "UNINTERRUPTIBLE";
	case 0x0004: return "STOPPED";
	case 0x0008: return "TRACED";
	default:     return "UNKNOWN";
	}
}

static void test_pid(pid_t pid, const char *description)
{
	struct procinfo info;
	long ret;

	memset(&info, 0, sizeof(info));
	ret = syscall(__NR_procinfo, pid, &info);

	printf("  %-24s (pid=%d):\n", description, pid);

	if (ret < 0) {
		printf("    Error: %s (errno=%d)\n\n", strerror(errno), errno);
		return;
	}

	printf("    PID:     %d\n", info.pid);
	printf("    PPID:    %d\n", info.ppid);
	printf("    State:   %ld (%s)\n", info.state, state_name(info.state));
	printf("    VSize:   %lu KB\n", info.vsize_kb);
	printf("    RSS:     %lu pages (%lu KB)\n",
	       info.rss_pages, info.rss_pages * 4);
	printf("    Command: %s\n\n", info.comm);
}

int main(void)
{
	long ret;

	printf("=== sys_procinfo Test ===\n\n");
	printf("Syscall number: %d\n\n", __NR_procinfo);

	/* Test 1: Query ourselves */
	test_pid(getpid(), "Self (this process)");

	/* Test 2: Query init (PID 1) */
	test_pid(1, "Init process");

	/* Test 3: Invalid PID (should return -ESRCH) */
	test_pid(999999, "Invalid PID");

	/* Test 4: PID 0 (idle/swapper - may or may not work) */
	test_pid(0, "PID 0 (swapper)");

	/* Test 5: NULL pointer (should return -EFAULT) */
	printf("  %-24s:\n", "NULL pointer test");
	ret = syscall(__NR_procinfo, getpid(), NULL);
	printf("    Result: returned %ld, errno=%d (%s)\n",
	       ret, errno, ret < 0 ? strerror(errno) : "success");
	printf("    Expected: -EFAULT (errno=14)\n\n");

	/*
	 * TODO: Add tests for:
	 *   - A process you don't own (test -EPERM with ptrace_may_access)
	 *   - Negative PID values
	 */

	printf("=== Done ===\n");
	return 0;
}
