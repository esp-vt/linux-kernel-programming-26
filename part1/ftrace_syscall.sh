#!/bin/bash
#
# ftrace_syscall.sh - Trace the kernel syscall dispatch path using ftrace
#
# Part 1, Task 1.2 of P1: System Calls - From Theory to Practice
# CS 5264/4224 & ECE 5414/4414 - Linux Kernel Programming, Spring 2026
#
# ftrace is the kernel's built-in function tracer.  It lives entirely in
# tracefs and requires no extra tools, just echo commands.
#
# This script sets up the function_graph tracer to record function calls
# inside syscall handlers such as __x64_sys_read and __x64_sys_write.
# The output shows the call tree with timing.
#
# Note: In Linux 6.x, the syscall entry path (do_syscall_64 and
# entry_SYSCALL_64) is marked "noinstr" and cannot be traced with ftrace.
# Instead, we trace the individual syscall handlers that do_syscall_64
# dispatches to, which shows the kernel's internal execution clearly.
#
# Usage (must be root):
#   sudo ./ftrace_syscall.sh
#
# Output:
#   ftrace-output.txt  - raw ftrace trace in the current directory
#
# After running, examine the output and answer the questions in the spec.

set -euo pipefail

TRACEDIR="/sys/kernel/tracing"
OUTFILE="ftrace-output.txt"

# --- Safety check -----------------------------------------------------------
if [[ $EUID -ne 0 ]]; then
    echo "ERROR: ftrace requires root.  Run with: sudo $0"
    exit 1
fi

if [[ ! -d "$TRACEDIR" ]]; then
    echo "ERROR: tracefs not mounted at $TRACEDIR"
    echo "Try: mount -t tracefs nodev /sys/kernel/tracing"
    exit 1
fi

# Check if function_graph tracer is available
TRACERS=$(cat "$TRACEDIR/available_tracers")
if ! echo "$TRACERS" | grep -q function_graph; then
    echo "ERROR: function_graph tracer is not available."
    echo "Your kernel needs CONFIG_FUNCTION_TRACER=y and CONFIG_FUNCTION_GRAPH_TRACER=y."
    echo "Available tracers: $TRACERS"
    exit 1
fi

# --- Reset any previous tracing state ---------------------------------------
echo nop   > "$TRACEDIR/current_tracer"
echo       > "$TRACEDIR/set_graph_function"
echo 0     > "$TRACEDIR/tracing_on"
echo       > "$TRACEDIR/trace"

echo "[*] ftrace reset complete"

# --- Configure function_graph tracer ----------------------------------------
# function_graph records entry/exit of every function, showing the full
# call tree with duration annotations.
#
# We trace multiple syscall handlers to capture a variety of kernel paths.
# Each __x64_sys_* function is the entry point that do_syscall_64 dispatches
# to after looking up the syscall number in sys_call_table[].

echo function_graph > "$TRACEDIR/current_tracer"

# Trace several common syscall handlers.
# Note: some simple syscalls (e.g., getpid) may be inlined and thus
# not available in the filter list.  We trace the ones that are available.
FUNCS="__x64_sys_read __x64_sys_write __x64_sys_openat __x64_sys_close __x64_sys_newfstatat __x64_sys_getdents64"
for fn in $FUNCS; do
    if grep -q "^${fn}$" "$TRACEDIR/available_filter_functions" 2>/dev/null; then
        echo "$fn" >> "$TRACEDIR/set_graph_function"
    fi
done

# Also increase the tracing depth for a good view
echo 10 > "$TRACEDIR/max_graph_depth"

echo "[*] Tracer configured: function_graph on syscall handlers"
echo "    (__x64_sys_read, __x64_sys_write, __x64_sys_openat, etc.)"

# --- Capture a burst of syscalls --------------------------------------------
echo 1 > "$TRACEDIR/tracing_on"

# Generate a mix of syscalls for the trace
echo "[*] Generating syscalls..."
ls /tmp > /dev/null 2>&1
cat /proc/uptime > /dev/null 2>&1
uname -a > /dev/null 2>&1

echo 0 > "$TRACEDIR/tracing_on"

echo "[*] Tracing stopped"

# --- Save output -------------------------------------------------------------
cat "$TRACEDIR/trace" > "$OUTFILE"
LINES=$(wc -l < "$OUTFILE")
echo "[*] Saved $LINES lines to $OUTFILE"

# --- Also capture syscall enter/exit events for entry/exit phase view --------
# The syscall tracepoints show the entry (with arguments) and exit (with
# return value) of each syscall, complementing the function_graph trace.
echo ""
echo "[*] Bonus: capturing syscall enter/exit tracepoints..."

echo nop > "$TRACEDIR/current_tracer"
echo > "$TRACEDIR/set_graph_function"
echo > "$TRACEDIR/trace"

# Enable syscall enter/exit tracepoints for read and write
echo 1 > "$TRACEDIR/events/syscalls/sys_enter_read/enable"
echo 1 > "$TRACEDIR/events/syscalls/sys_exit_read/enable"
echo 1 > "$TRACEDIR/events/syscalls/sys_enter_write/enable"
echo 1 > "$TRACEDIR/events/syscalls/sys_exit_write/enable"

echo 1 > "$TRACEDIR/tracing_on"
cat /proc/uptime > /dev/null 2>&1
echo 0 > "$TRACEDIR/tracing_on"

# Append to output
echo "" >> "$OUTFILE"
echo "=== Syscall Enter/Exit Tracepoints ===" >> "$OUTFILE"
cat "$TRACEDIR/trace" >> "$OUTFILE"

# Disable events
echo 0 > "$TRACEDIR/events/syscalls/sys_enter_read/enable"
echo 0 > "$TRACEDIR/events/syscalls/sys_exit_read/enable"
echo 0 > "$TRACEDIR/events/syscalls/sys_enter_write/enable"
echo 0 > "$TRACEDIR/events/syscalls/sys_exit_write/enable"

LINES=$(wc -l < "$OUTFILE")
echo "[*] Final output: $LINES lines in $OUTFILE"

# --- Clean up ----------------------------------------------------------------
echo nop > "$TRACEDIR/current_tracer"
echo     > "$TRACEDIR/set_graph_function"

echo ""
echo "=== Next Steps ==="
echo "1. Open $OUTFILE and examine the call tree"
echo "2. Look for the dispatch phase: each __x64_sys_* function is the"
echo "   handler that do_syscall_64() calls after looking up sys_call_table[]."
echo "3. Identify key subsystems: VFS (vfs_read), security (security_file_permission),"
echo "   filesystem (ext4_file_read_iter), page cache (filemap_read)"
echo "4. Search for Spectre mitigation evidence:"
echo "   grep '_nospec\|array_index' $OUTFILE"
echo "   (These may appear in the syscall table lookup path)"
echo ""
