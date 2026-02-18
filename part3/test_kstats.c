/*
 * test_kstats.c - Test program for the sys_kstats system call
 *
 * Part 3, Task 3.2 of P1: System Calls - From Theory to Practice
 * CS 5264/4224 & ECE 5414/4414 - Linux Kernel Programming, Spring 2026
 *
 * This program calls your custom sys_kstats() syscall and prints the
 * results.  It also reads /proc files to cross-check the values.
 *
 * Compile:
 *   make part3
 *   (or: gcc -O2 -Wall -o test_kstats test_kstats.c)
 *
 * Run (inside your Linux VM with your custom kernel):
 *   ./test_kstats
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "kstats.h"

/*
 * Helper: read a numeric value from a /proc file.
 * Searches for a line starting with 'key' and returns the first
 * integer on that line.  Returns -1 on failure.
 */
static long read_proc_value(const char *path, const char *key)
{
	FILE *fp;
	char line[256];

	fp = fopen(path, "r");
	if (!fp)
		return -1;

	while (fgets(line, sizeof(line), fp)) {
		if (strncmp(line, key, strlen(key)) == 0) {
			char *p = line + strlen(key);
			/* Skip to the first digit */
			while (*p && (*p < '0' || *p > '9'))
				p++;
			fclose(fp);
			return atol(p);
		}
	}
	fclose(fp);
	return -1;
}

static void print_separator(void)
{
	printf("  %-28s %-16s %-16s\n",
	       "----------------------------",
	       "----------------",
	       "----------------");
}

int main(void)
{
	struct kstats ks;
	long ret;

	printf("=== sys_kstats Test ===\n\n");
	printf("Syscall number: %d\n\n", __NR_kstats);

	/* --- Call sys_kstats ------------------------------------------------ */
	ret = syscall(__NR_kstats, &ks);
	if (ret < 0) {
		fprintf(stderr, "sys_kstats failed: %s (errno=%d)\n",
			strerror(errno), errno);
		fprintf(stderr, "\nCommon causes:\n");
		fprintf(stderr, "  - Syscall not registered (wrong number?)\n");
		fprintf(stderr, "  - Kernel not rebuilt after adding syscall\n");
		fprintf(stderr, "  - Not running inside the custom kernel\n");
		return 1;
	}

	/* --- Print syscall results ------------------------------------------ */
	printf("sys_kstats() returned successfully:\n");
	printf("  Processes:        %lu (running: %lu)\n",
	       ks.nr_processes, ks.nr_running);
	printf("  Total memory:     %lu KB\n", ks.total_memory_kb);
	printf("  Free memory:      %lu KB\n", ks.free_memory_kb);
	printf("  Buffer memory:    %lu KB\n", ks.buffer_memory_kb);
	printf("  Uptime:           %lu seconds\n", ks.uptime_seconds);
	printf("  Context switches: %lu\n", ks.context_switches);

	/* --- Cross-check with /proc ----------------------------------------- */
	printf("\n=== Cross-Check with /proc ===\n\n");
	printf("  %-28s %-16s %-16s\n", "Field", "sys_kstats", "/proc");
	print_separator();

	long proc_total  = read_proc_value("/proc/meminfo", "MemTotal:");
	long proc_free   = read_proc_value("/proc/meminfo", "MemFree:");
	long proc_buffer = read_proc_value("/proc/meminfo", "Buffers:");
	long proc_ctxt   = read_proc_value("/proc/stat", "ctxt");

	printf("  %-28s %-16lu %-16ld\n", "Total memory (KB)",
	       ks.total_memory_kb, proc_total);
	printf("  %-28s %-16lu %-16ld\n", "Free memory (KB)",
	       ks.free_memory_kb, proc_free);
	printf("  %-28s %-16lu %-16ld\n", "Buffer memory (KB)",
	       ks.buffer_memory_kb, proc_buffer);
	printf("  %-28s %-16lu %-16ld\n", "Context switches",
	       ks.context_switches, proc_ctxt);

	/* Uptime from /proc/uptime (first field, in seconds with decimal) */
	{
		FILE *fp = fopen("/proc/uptime", "r");
		double uptime_proc = 0;
		if (fp) {
			fscanf(fp, "%lf", &uptime_proc);
			fclose(fp);
		}
		printf("  %-28s %-16lu %-16.0f\n", "Uptime (seconds)",
		       ks.uptime_seconds, uptime_proc);
	}

	/* --- Error handling tests ------------------------------------------- */
	printf("\n=== Error Handling Tests ===\n\n");

	ret = syscall(__NR_kstats, NULL);
	printf("  NULL pointer:  returned %ld, errno=%d (%s)\n",
	       ret, errno, ret < 0 ? strerror(errno) : "success");
	printf("  Expected:      returned -1, errno=14 (Bad address / EFAULT)\n");

	/*
	 * TODO: Write your analysis in kstats-comparison.md:
	 *
	 * 1. Do the sys_kstats values match /proc?  Explain any differences.
	 * 2. When would a single syscall be preferred over parsing /proc files?
	 * 3. Are all fields from the same instant?  What atomicity issues exist?
	 */

	printf("\n=== Done ===\n");
	return 0;
}
