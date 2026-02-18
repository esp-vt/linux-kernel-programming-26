/*
 * kstats.h - Shared struct definitions for sys_kstats and sys_procinfo
 *
 * Part 3 of P1: System Calls - From Theory to Practice
 * CS 5264/4224 & ECE 5414/4414 - Linux Kernel Programming, Spring 2026
 *
 * This header is shared between:
 *   - The kernel implementation (kernel/kstats.c)
 *   - The userspace test programs (test_kstats.c, test_procinfo.c)
 *
 * In a real kernel patch, this would go in include/uapi/linux/kstats.h
 * so that both kernel and userspace can include it.  For this project,
 * we keep a local copy for the test programs.
 *
 * IMPORTANT: The struct layout here MUST match what the kernel uses.
 * If you change the kernel struct, update this file to match.
 */

#ifndef _KSTATS_H
#define _KSTATS_H

/*
 * TODO: Update these syscall numbers to match what you added to
 * arch/x86/entry/syscalls/syscall_64.tbl.  The numbers below are
 * suggestions; use the next available number in your kernel.
 */
#define __NR_kstats        470
#define __NR_procinfo      471
#define __NR_null_syscall  472

/*
 * struct kstats - Aggregated kernel statistics
 *
 * Returned by sys_kstats().  Each field maps to information that
 * would otherwise require parsing /proc/stat, /proc/meminfo,
 * and /proc/uptime separately.
 */
struct kstats {
	unsigned long nr_processes;     /* Total number of processes       */
	unsigned long nr_running;       /* Currently runnable              */
	unsigned long total_memory_kb;  /* Total physical RAM in KB        */
	unsigned long free_memory_kb;   /* Free (unused) RAM in KB         */
	unsigned long buffer_memory_kb; /* Buffer cache in KB              */
	unsigned long uptime_seconds;   /* Seconds since boot              */
	unsigned long context_switches; /* Total voluntary + involuntary   */
};

/*
 * struct procinfo - Per-process information
 *
 * Returned by sys_procinfo().  Provides a snapshot of a single
 * process identified by PID.
 *
 * Error codes:
 *   -ESRCH   PID does not exist
 *   -EFAULT  Invalid userspace pointer
 *   -EPERM   Caller lacks permission to inspect this process
 */
struct procinfo {
	int    pid;            /* Process ID                            */
	int    ppid;           /* Parent process ID                     */
	long   state;          /* Task state (TASK_RUNNING, etc.)       */
	unsigned long vsize_kb;    /* Virtual memory size in KB         */
	unsigned long rss_pages;   /* Resident set size in pages        */
	char   comm[16];       /* Process name (from task->comm)        */
};

#endif /* _KSTATS_H */
