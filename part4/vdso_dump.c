/*
 * vdso_dump.c - Find and extract the vDSO shared object
 *
 * Part 4, Task 4.3 of P1: System Calls - From Theory to Practice
 * CS 5264/4224 & ECE 5414/4414 - Linux Kernel Programming, Spring 2026
 *
 * The vDSO (virtual Dynamic Shared Object) is a small shared library
 * that the kernel maps into every process's address space.  It contains
 * implementations of time-related syscalls that can run entirely in
 * user mode by reading the kernel's vvar page.
 *
 * This program:
 *   1. Reads /proc/self/maps to find the [vdso] region
 *   2. Prints the address range and size
 *   3. Extracts the vDSO to a file so you can inspect it with objdump
 *
 * Build:
 *   make part4
 *   (or: gcc -O2 -Wall -o vdso_dump vdso_dump.c)
 *
 * Run:
 *   ./vdso_dump
 *   objdump -T vdso.so          # list exported symbols
 *   objdump -d vdso.so          # disassemble
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int main(void)
{
	FILE *fp;
	char line[512];
	unsigned long vdso_start = 0, vdso_end = 0;
	unsigned long vvar_start = 0, vvar_end = 0;

	printf("=== vDSO Finder ===\n\n");

	/* --- Step 1: Find [vdso] and [vvar] in /proc/self/maps ------------- */
	fp = fopen("/proc/self/maps", "r");
	if (!fp) {
		perror("fopen /proc/self/maps");
		return 1;
	}

	while (fgets(line, sizeof(line), fp)) {
		if (strstr(line, "[vdso]")) {
			sscanf(line, "%lx-%lx", &vdso_start, &vdso_end);
			printf("  [vdso]  0x%lx - 0x%lx  (%lu bytes)\n",
			       vdso_start, vdso_end, vdso_end - vdso_start);
		}
		if (strstr(line, "[vvar]")) {
			sscanf(line, "%lx-%lx", &vvar_start, &vvar_end);
			printf("  [vvar]  0x%lx - 0x%lx  (%lu bytes)\n",
			       vvar_start, vvar_end, vvar_end - vvar_start);
		}
	}
	fclose(fp);

	if (vdso_start == 0) {
		fprintf(stderr, "ERROR: Could not find [vdso] mapping.\n");
		return 1;
	}

	/* --- Step 2: Extract the vDSO to a file ----------------------------- */
	size_t vdso_size = vdso_end - vdso_start;
	const char *outfile = "vdso.so";

	FILE *out = fopen(outfile, "wb");
	if (!out) {
		perror("fopen vdso.so");
		return 1;
	}

	/*
	 * The vDSO is mapped in our address space, so we can just
	 * memcpy it out.  No need for /proc/self/mem tricks.
	 */
	fwrite((void *)vdso_start, 1, vdso_size, out);
	fclose(out);

	printf("\n  Extracted vDSO to: %s (%zu bytes)\n", outfile, vdso_size);

	/* --- Step 3: Print analysis instructions ---------------------------- */
	printf("\n=== Next Steps ===\n");
	printf("  1. List vDSO symbols:\n");
	printf("       objdump -T vdso.so\n");
	printf("     Look for: __vdso_gettimeofday, __vdso_clock_gettime\n\n");
	printf("  2. Disassemble a function:\n");
	printf("       objdump -d vdso.so | grep -A 30 '__vdso_gettimeofday'\n\n");
	printf("  3. Answer in vdso-analysis.md:\n");
	printf("     - How does the vDSO read kernel time without a trap?\n");
	printf("     - What data lives in the vvar page?\n");
	printf("     - Why can't all syscalls use vDSO?\n");

	return 0;
}
