/*
 * syscall_bench.c - Comprehensive syscall latency benchmark
 *
 * Part 4 of P1: System Calls - From Theory to Practice
 * CS 5264/4224 & ECE 5414/4414 - Linux Kernel Programming, Spring 2026
 *
 * Measures the latency of various syscall interfaces using rdtscp with
 * proper statistical methodology: warmup, multiple trials, and summary
 * statistics.  This lets you compare the cost of:
 *   - A no-op syscall (pure trap overhead)
 *   - Real syscalls of varying complexity
 *   - vDSO-accelerated calls (no kernel trap at all)
 *
 * Build:
 *   make part4
 *   (or: gcc -O2 -Wall -lm -o syscall_bench syscall_bench.c)
 *
 * Run (pin to one CPU for consistent results):
 *   taskset -c 0 ./syscall_bench
 *
 * Prerequisites:
 *   - Custom kernel with sys_null (472) and sys_kstats (470)
 *   - Running inside your Linux VM with that kernel
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>

#include "../part3/kstats.h"   /* struct kstats, syscall numbers */

/* ================================================================
 * Configuration
 * ================================================================ */

#define WARMUP_ITERS    100       /* Warmup iterations (fill caches)  */
#define BENCH_ITERS     100000    /* Iterations per trial             */
#define NUM_TRIALS      10        /* Independent trials               */

/* ================================================================
 * Timing helpers
 * ================================================================ */

/*
 * rdtscp - Read Time Stamp Counter (serializing)
 *
 * The RDTSCP instruction reads the 64-bit TSC and the IA32_TSC_AUX
 * MSR atomically.  Unlike RDTSC, it waits for all prior instructions
 * to retire before reading---this prevents out-of-order execution
 * from skewing the measurement.
 */
static inline uint64_t rdtscp(void)
{
	uint32_t lo, hi, aux;

	asm volatile("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux));
	return ((uint64_t)hi << 32) | lo;
}

/*
 * cpuid_barrier - Full serialization barrier
 *
 * CPUID is a serializing instruction: the CPU will not execute any
 * subsequent instruction until CPUID completes and all prior
 * instructions have retired.  We use it before RDTSCP to ensure
 * the benchmark loop is fully contained between the two timestamps.
 */
static inline void cpuid_barrier(void)
{
	uint32_t a, b, c, d;

	asm volatile("cpuid"
		     : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
		     : "a"(0) : "memory");
}

/*
 * Get CPU frequency from sysfs or /proc/cpuinfo.
 * Returns frequency in Hz, or 0 on failure.
 */
static uint64_t get_cpu_freq_hz(void)
{
	FILE *fp;
	char buf[256];
	double mhz = 0;

	/* Try sysfs (more reliable on modern kernels) */
	fp = fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r");
	if (fp) {
		if (fgets(buf, sizeof(buf), fp)) {
			uint64_t khz = strtoull(buf, NULL, 10);
			fclose(fp);
			return khz * 1000ULL;
		}
		fclose(fp);
	}

	/* Fallback: /proc/cpuinfo */
	fp = fopen("/proc/cpuinfo", "r");
	if (!fp)
		return 0;

	while (fgets(buf, sizeof(buf), fp)) {
		if (strncmp(buf, "cpu MHz", 7) == 0) {
			char *colon = strchr(buf, ':');
			if (colon)
				mhz = atof(colon + 1);
			break;
		}
	}
	fclose(fp);

	return (uint64_t)(mhz * 1e6);
}

/* Convert TSC cycles to nanoseconds */
static double cycles_to_ns(uint64_t cycles, uint64_t freq_hz)
{
	return (double)cycles * 1e9 / (double)freq_hz;
}

/* ================================================================
 * Statistics
 * ================================================================ */

struct bench_stats {
	double avg_ns;
	double min_ns;
	double max_ns;
	double stddev_ns;
};

static struct bench_stats compute_stats(double *results, int n)
{
	struct bench_stats s;
	double sum = 0, sum_sq = 0;

	s.min_ns = results[0];
	s.max_ns = results[0];

	for (int i = 0; i < n; i++) {
		sum += results[i];
		sum_sq += results[i] * results[i];
		if (results[i] < s.min_ns)
			s.min_ns = results[i];
		if (results[i] > s.max_ns)
			s.max_ns = results[i];
	}

	s.avg_ns = sum / n;
	s.stddev_ns = sqrt(sum_sq / n - s.avg_ns * s.avg_ns);

	return s;
}

static void print_result(const char *name, struct bench_stats *s)
{
	printf("  %-35s %8.1f  %8.1f  %8.1f  %7.1f\n",
	       name, s->avg_ns, s->min_ns, s->max_ns, s->stddev_ns);
}

/* ================================================================
 * Benchmark: sys_null (pure syscall overhead)
 *
 * This is IMPLEMENTED as an example.  Follow this pattern for the
 * remaining benchmarks below.
 * ================================================================ */

static struct bench_stats bench_null_syscall(uint64_t freq_hz)
{
	double results[NUM_TRIALS];

	/* Warmup: prime caches and branch predictor */
	for (int i = 0; i < WARMUP_ITERS; i++)
		syscall(__NR_null_syscall);

	/* Measurement trials */
	for (int t = 0; t < NUM_TRIALS; t++) {
		uint64_t start, end;

		cpuid_barrier();
		start = rdtscp();

		for (int i = 0; i < BENCH_ITERS; i++)
			syscall(__NR_null_syscall);

		cpuid_barrier();
		end = rdtscp();

		results[t] = cycles_to_ns((end - start) / BENCH_ITERS,
					  freq_hz);
	}

	return compute_stats(results, NUM_TRIALS);
}

/* ================================================================
 * TODO: Implement the remaining benchmark functions
 *
 * Each function follows the same pattern as bench_null_syscall():
 *   1. Warmup loop
 *   2. For each trial:
 *      a. cpuid_barrier(); start = rdtscp();
 *      b. Tight loop of BENCH_ITERS calls
 *      c. cpuid_barrier(); end = rdtscp();
 *      d. results[t] = cycles_to_ns(...)
 *   3. return compute_stats(results, NUM_TRIALS);
 * ================================================================ */

/* Benchmark: syscall(__NR_getpid)  -- minimal-work real syscall */
static struct bench_stats bench_getpid_syscall(uint64_t freq_hz)
{
	double results[NUM_TRIALS];

	for (int i = 0; i < WARMUP_ITERS; i++)
		syscall(__NR_getpid);

	for (int t = 0; t < NUM_TRIALS; t++) {
		uint64_t start, end;

		cpuid_barrier();
		start = rdtscp();

		for (int i = 0; i < BENCH_ITERS; i++)
			syscall(__NR_getpid);

		cpuid_barrier();
		end = rdtscp();

		results[t] = cycles_to_ns((end - start) / BENCH_ITERS, freq_hz);
	}

	return compute_stats(results, NUM_TRIALS);
}

/* Benchmark: getpid()  -- libc wrapper (glibc may cache the result!) */
static struct bench_stats bench_getpid_libc(uint64_t freq_hz)
{
	double results[NUM_TRIALS];

	for (int i = 0; i < WARMUP_ITERS; i++)
		getpid();

	for (int t = 0; t < NUM_TRIALS; t++) {
		uint64_t start, end;

		cpuid_barrier();
		start = rdtscp();

		for (int i = 0; i < BENCH_ITERS; i++)
			getpid();

		cpuid_barrier();
		end = rdtscp();

		results[t] = cycles_to_ns((end - start) / BENCH_ITERS, freq_hz);
	}

	return compute_stats(results, NUM_TRIALS);
}

/* Benchmark: syscall(__NR_gettimeofday, ...)  -- real syscall path */
static struct bench_stats bench_gtod_syscall(uint64_t freq_hz)
{
	double results[NUM_TRIALS];
	struct timeval tv;

	for (int i = 0; i < WARMUP_ITERS; i++)
		syscall(__NR_gettimeofday, &tv, NULL);

	for (int t = 0; t < NUM_TRIALS; t++) {
		uint64_t start, end;

		cpuid_barrier();
		start = rdtscp();

		for (int i = 0; i < BENCH_ITERS; i++)
			syscall(__NR_gettimeofday, &tv, NULL);

		cpuid_barrier();
		end = rdtscp();

		results[t] = cycles_to_ns((end - start) / BENCH_ITERS, freq_hz);
	}

	return compute_stats(results, NUM_TRIALS);
}

/* Benchmark: gettimeofday()  -- vDSO-accelerated (no kernel trap) */
static struct bench_stats bench_gtod_vdso(uint64_t freq_hz)
{
	double results[NUM_TRIALS];
	struct timeval tv;

	for (int i = 0; i < WARMUP_ITERS; i++)
		gettimeofday(&tv, NULL);

	for (int t = 0; t < NUM_TRIALS; t++) {
		uint64_t start, end;

		cpuid_barrier();
		start = rdtscp();

		for (int i = 0; i < BENCH_ITERS; i++)
			gettimeofday(&tv, NULL);

		cpuid_barrier();
		end = rdtscp();

		results[t] = cycles_to_ns((end - start) / BENCH_ITERS, freq_hz);
	}

	return compute_stats(results, NUM_TRIALS);
}

/* Benchmark: clock_gettime(CLOCK_MONOTONIC)  -- vDSO-accelerated */
static struct bench_stats bench_clock_gettime_vdso(uint64_t freq_hz)
{
	double results[NUM_TRIALS];
	struct timespec ts;

	for (int i = 0; i < WARMUP_ITERS; i++)
		clock_gettime(CLOCK_MONOTONIC, &ts);

	for (int t = 0; t < NUM_TRIALS; t++) {
		uint64_t start, end;

		cpuid_barrier();
		start = rdtscp();

		for (int i = 0; i < BENCH_ITERS; i++)
			clock_gettime(CLOCK_MONOTONIC, &ts);

		cpuid_barrier();
		end = rdtscp();

		results[t] = cycles_to_ns((end - start) / BENCH_ITERS, freq_hz);
	}

	return compute_stats(results, NUM_TRIALS);
}

/* Benchmark: syscall(__NR_kstats, ...)  -- your custom syscall */
static struct bench_stats bench_kstats(uint64_t freq_hz)
{
	double results[NUM_TRIALS];
	struct kstats ks;

	for (int i = 0; i < WARMUP_ITERS; i++)
		syscall(__NR_kstats, &ks);

	for (int t = 0; t < NUM_TRIALS; t++) {
		uint64_t start, end;

		cpuid_barrier();
		start = rdtscp();

		for (int i = 0; i < BENCH_ITERS; i++)
			syscall(__NR_kstats, &ks);

		cpuid_barrier();
		end = rdtscp();

		results[t] = cycles_to_ns((end - start) / BENCH_ITERS, freq_hz);
	}

	return compute_stats(results, NUM_TRIALS);
}

/* ================================================================
 * Main
 * ================================================================ */

int main(void)
{
	uint64_t freq_hz = get_cpu_freq_hz();
	struct bench_stats s;

	if (freq_hz == 0) {
		fprintf(stderr, "ERROR: Could not determine CPU frequency.\n");
		fprintf(stderr, "Set manually in get_cpu_freq_hz() and recompile.\n");
		return 1;
	}

	printf("=== Syscall Latency Benchmark ===\n\n");
	printf("CPU frequency:  %.2f GHz\n", freq_hz / 1e9);
	printf("Warmup:         %d iterations\n", WARMUP_ITERS);
	printf("Measurement:    %d iterations x %d trials\n\n",
	       BENCH_ITERS, NUM_TRIALS);

	printf("  %-35s %8s  %8s  %8s  %7s\n",
	       "Test", "Avg(ns)", "Min(ns)", "Max(ns)", "StdDev");
	printf("  %-35s %8s  %8s  %8s  %7s\n",
	       "-----------------------------------",
	       "--------", "--------", "--------", "-------");

	s = bench_null_syscall(freq_hz);
	print_result("1. sys_null (pure overhead)", &s);

	s = bench_getpid_syscall(freq_hz);
	print_result("2. syscall(__NR_getpid)", &s);

	s = bench_getpid_libc(freq_hz);
	print_result("3. getpid() [libc]", &s);

	s = bench_gtod_syscall(freq_hz);
	print_result("4. syscall(gettimeofday)", &s);

	s = bench_gtod_vdso(freq_hz);
	print_result("5. gettimeofday() [vDSO]", &s);

	s = bench_clock_gettime_vdso(freq_hz);
	print_result("6. clock_gettime() [vDSO]", &s);

	s = bench_kstats(freq_hz);
	print_result("7. sys_kstats (your syscall)", &s);

	printf("\nNotes:\n");
	printf("  - Pin to one CPU:  taskset -c 0 ./syscall_bench\n");
	printf("  - Disable turbo:   echo 1 > /sys/.../no_turbo (optional)\n");
	printf("  - Record results in part4/results.md\n");

	return 0;
}
