# ===========================================================================
# P1 Starter Code Makefile
# System Calls --- From Theory to Practice
#
# CS 5264/4224 & ECE 5414/4414 - Linux Kernel Programming, Spring 2026
#
# Usage:
#   make              Build all userspace programs (Parts 2-5)
#   make part2        Build Part 2 programs only
#   make part3        Build Part 3 test programs only
#   make part4        Build Part 4 benchmark + vdso_dump
#   make part5        Build Part 5 security programs
#   make clean        Remove all compiled binaries
#   make help         Show this help message
#   make submit       Package your submission (update PID first)
#
# Note: Part 1 uses shell scripts and strace, no compilation needed.
#       Part 3 kernel code (kstats.c) is built with the kernel, not here.
#       This Makefile builds ONLY the userspace test/demo programs.
# ===========================================================================

CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -Wno-unused-parameter
LDFLAGS =

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------

.PHONY: all part2 part3 part4 part5 clean help submit

all: part2 part3 part4 part5
	@echo ""
	@echo "=== All userspace programs built successfully ==="
	@echo "Run inside your Linux VM with the custom kernel."
	@echo ""

# --- Part 2: GDB Exploration -----------------------------------------------

part2: part2/test_syscall
	@echo "[Part 2] Built: test_syscall"

# Compile with -O0 -g so GDB can inspect variables and registers clearly.
# Optimization would reorder or eliminate our syscall calls.
part2/test_syscall: part2/test_syscall.c
	$(CC) -O0 -g -Wall -o $@ $<

# --- Part 3: Custom Syscall Test Programs -----------------------------------

PART3_BINS = part3/test_kstats part3/test_procinfo

part3: $(PART3_BINS)
	@echo "[Part 3] Built: test_kstats test_procinfo"

part3/test_kstats: part3/test_kstats.c part3/kstats.h
	$(CC) $(CFLAGS) -o $@ $<

part3/test_procinfo: part3/test_procinfo.c part3/kstats.h
	$(CC) $(CFLAGS) -o $@ $<

# --- Part 4: Syscall Performance Benchmark ----------------------------------

PART4_BINS = part4/syscall_bench part4/vdso_dump

part4: $(PART4_BINS)
	@echo "[Part 4] Built: syscall_bench vdso_dump"

part4/syscall_bench: part4/syscall_bench.c part3/kstats.h
	$(CC) $(CFLAGS) -o $@ $< -lm

part4/vdso_dump: part4/vdso_dump.c
	$(CC) $(CFLAGS) -o $@ $<

# --- Part 5: Security Demos ------------------------------------------------

PART5_BINS = part5/seccomp_strict part5/seccomp_filter part5/kpti_test

part5: $(PART5_BINS)
	@echo "[Part 5] Built: seccomp_strict seccomp_filter kpti_test"

part5/seccomp_strict: part5/seccomp_strict.c
	$(CC) $(CFLAGS) -o $@ $<

part5/seccomp_filter: part5/seccomp_filter.c part3/kstats.h
	$(CC) $(CFLAGS) -o $@ $<

part5/kpti_test: part5/kpti_test.c
	$(CC) $(CFLAGS) -o $@ $<

# --- Utility ----------------------------------------------------------------

clean:
	rm -f part2/test_syscall
	rm -f $(PART3_BINS)
	rm -f $(PART4_BINS)
	rm -f $(PART5_BINS)
	rm -f part4/vdso.so
	@echo "Cleaned all binaries."

# Update YOUR_PID before running 'make submit'
YOUR_PID ?= CHANGEME

submit:
	@if [ "$(YOUR_PID)" = "CHANGEME" ]; then \
		echo "ERROR: Set your PID first:"; \
		echo "  make submit YOUR_PID=your_pid_here"; \
		exit 1; \
	fi
	@echo "Packaging submission..."
	cd .. && tar czf $(YOUR_PID)-lkp26-p1.tar.gz \
		--exclude='*.o' --exclude='__pycache__' \
		p1/
	@echo "Created: ../$(YOUR_PID)-lkp26-p1.tar.gz"

help:
	@echo ""
	@echo "P1 Starter Code - Available Targets"
	@echo "===================================="
	@echo ""
	@echo "  make          Build all userspace programs"
	@echo "  make part2    Part 2: test_syscall (for GDB exploration)"
	@echo "  make part3    Part 3: test_kstats, test_procinfo"
	@echo "  make part4    Part 4: syscall_bench, vdso_dump"
	@echo "  make part5    Part 5: seccomp_strict, seccomp_filter, kpti_test"
	@echo "  make clean    Remove compiled binaries"
	@echo "  make submit   Package submission tarball"
	@echo ""
	@echo "Part 1 uses scripts (no compilation needed):"
	@echo "  sudo ./part1/ftrace_syscall.sh"
	@echo ""
	@echo "Part 2 also has a helper script:"
	@echo "  sudo ./part2/check_msr.sh"
	@echo ""
