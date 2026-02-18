/*
 * seccomp_strict.c - Demonstrate seccomp strict mode
 *
 * Part 5, Task 5.1 of P1: System Calls - From Theory to Practice
 * CS 5264/4224 & ECE 5414/4414 - Linux Kernel Programming, Spring 2026
 *
 * seccomp (secure computing) restricts which system calls a process
 * can make.  In STRICT mode, only four syscalls are allowed:
 *   read(2), write(2), exit(2), sigreturn(2)
 *
 * Any other syscall immediately kills the process with SIGKILL.
 * This is the simplest sandboxing mechanism in Linux.
 *
 * Build:
 *   make part5
 *   (or: gcc -O2 -Wall -o seccomp_strict seccomp_strict.c)
 *
 * Run:
 *   ./seccomp_strict
 *
 * Expected output:
 *   - The read/write calls succeed
 *   - The getpid() call triggers SIGKILL
 *   - The process is killed (you'll see "Killed" from the shell)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <linux/seccomp.h>

int main(void)
{
	char buf[256];
	ssize_t n;
	int fd;

	printf("=== seccomp Strict Mode Demo ===\n\n");

	/*
	 * Open the file BEFORE enabling seccomp, because open() will
	 * be blocked once strict mode is active.
	 */
	fd = open("/etc/hostname", O_RDONLY);
	if (fd < 0) {
		/* Try a different file if /etc/hostname doesn't exist */
		fd = open("/etc/passwd", O_RDONLY);
		if (fd < 0) {
			perror("open");
			return 1;
		}
	}

	printf("[*] File descriptor %d opened before seccomp.\n", fd);
	printf("[*] Enabling seccomp STRICT mode...\n\n");

	/*
	 * Enable strict mode.  After this call, ONLY read, write, exit,
	 * and sigreturn are allowed.  Everything else = instant SIGKILL.
	 */
	if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT) < 0) {
		perror("prctl(PR_SET_SECCOMP)");
		return 1;
	}

	/*
	 * These are ALLOWED in strict mode:
	 */

	/* read() - allowed */
	n = read(fd, buf, sizeof(buf) - 1);
	if (n > 0) {
		buf[n] = '\0';
		/* write() to stdout - allowed */
		write(STDOUT_FILENO, "[+] read() succeeded: ", 22);
		write(STDOUT_FILENO, buf, n);
		write(STDOUT_FILENO, "\n", 1);
	}

	/*
	 * TODO: The call below will KILL this process.
	 *
	 * Uncomment the getpid() line.  When you run the program,
	 * the shell should print "Killed".  In dmesg or the audit
	 * log, you'll see a seccomp violation.
	 *
	 * Questions:
	 *   1. What signal kills the process?  (Hint: check dmesg)
	 *   2. How is SIGKILL different from returning EPERM?
	 *   3. Why must you open files BEFORE enabling strict mode?
	 */

	/* Uncomment this line to trigger the kill: */
	pid_t pid = getpid(); 
	printf("PID: %d\n", pid);  /* This line never executes */

	/*
	 * If you didn't uncomment getpid(), we can still exit cleanly
	 * because _exit() maps to the exit syscall, which is allowed.
	 */
	write(STDOUT_FILENO, "[*] Exiting normally.\n", 21);

	/* Use _exit to avoid libc cleanup that might call blocked syscalls */
	_exit(0);
}
