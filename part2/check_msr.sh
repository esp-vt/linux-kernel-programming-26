#!/bin/bash
#
# check_msr.sh - Verify that MSR_LSTAR points to entry_SYSCALL_64
#
# Part 2, Task 2.2 of P1: System Calls - From Theory to Practice
# CS 5264/4224 & ECE 5414/4414 - Linux Kernel Programming, Spring 2026
#
# Background:
#   The x86-64 SYSCALL instruction is a fast trap into the kernel.
#   When user code executes SYSCALL, the CPU:
#     1. Saves RIP (return address) into RCX
#     2. Saves RFLAGS into R11
#     3. Loads the new RIP from MSR_LSTAR (Model-Specific Register 0xC0000082)
#     4. Loads CS/SS from MSR_STAR
#
#   So MSR_LSTAR holds the kernel's syscall entry point.  On Linux x86-64,
#   this should be the address of entry_SYSCALL_64 (defined in
#   arch/x86/entry/entry_64.S).
#
# This script reads MSR_LSTAR and compares it with the symbol table.
#
# Prerequisites:
#   - msr-tools package:  apt install msr-tools
#   - msr kernel module:  modprobe msr
#   - Must be run as root
#
# Usage:
#   sudo ./check_msr.sh

set -euo pipefail

MSR_LSTAR=0xc0000082

if [[ $EUID -ne 0 ]]; then
    echo "ERROR: Reading MSRs requires root.  Run with: sudo $0"
    exit 1
fi

# Ensure the msr module is loaded
if [[ ! -e /dev/cpu/0/msr ]]; then
    echo "[*] Loading msr kernel module..."
    modprobe msr
    if [[ ! -e /dev/cpu/0/msr ]]; then
        echo "ERROR: Could not load msr module.  Is CONFIG_X86_MSR enabled?"
        exit 1
    fi
fi

# Check for rdmsr
if ! command -v rdmsr &>/dev/null; then
    echo "ERROR: rdmsr not found.  Install msr-tools:"
    echo "  apt install msr-tools    (Debian/Ubuntu)"
    echo "  dnf install msr-tools    (Fedora)"
    exit 1
fi

echo "=== MSR_LSTAR Verification ==="
echo ""

# Read MSR_LSTAR
LSTAR_VALUE=$(rdmsr "$MSR_LSTAR")
echo "MSR_LSTAR (0xC0000082) = 0x${LSTAR_VALUE}"

# Look up entry_SYSCALL_64 in the symbol table
echo ""
echo "Looking up entry_SYSCALL_64 in symbol tables..."

if [[ -f /proc/kallsyms ]]; then
    KALLSYM=$(grep ' entry_SYSCALL_64$' /proc/kallsyms 2>/dev/null || true)
    if [[ -n "$KALLSYM" ]]; then
        echo "  /proc/kallsyms:  $KALLSYM"
    fi
fi

SYSMAP="/boot/System.map-$(uname -r)"
if [[ -f "$SYSMAP" ]]; then
    MAPENTRY=$(grep ' entry_SYSCALL_64$' "$SYSMAP" 2>/dev/null || true)
    if [[ -n "$MAPENTRY" ]]; then
        echo "  System.map:      $MAPENTRY"
    fi
fi

echo ""
echo "=== Verification ==="
echo "If MSR_LSTAR matches the address of entry_SYSCALL_64, the SYSCALL"
echo "instruction will jump directly to the kernel's syscall entry code."
echo ""
echo "TODO: Compare the values above and confirm they match."
echo "      Record your findings in gdb-answers.md."
