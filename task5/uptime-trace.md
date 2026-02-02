## 5.1 Trace /proc/uptime

**Function that generates output:**

- Function: uptime_proc_show

```
~/linux$ grep -n "uptime" fs/proc/*.c
fs/proc/uptime.c:12:static int uptime_proc_show(struct seq_file *m, void *v)
```

- Location: fs/proc/uptime.c:12

**file_operations / show function:**

- Registration method:

It is registered using proc_create_single() function

```
fs/proc/uptime.c:45:	pde = proc_create_single("uptime", 0, NULL, uptime_proc_show);
```

- Location: fs/proc/uptime.c:45

**Call chain (VFS to output):**

1. vfs_read - fs/read_write.c

2. seq_read - fs/seq_file.c:151

3. seq_read_iter - fs/seq_file.c:171

4. copy_to_iter - fs/seq_file.c:216

**Uptime calculation:**

- Key function: ktime_get_boottime_ts64(&uptime)

- Variables used: struct timespec64 uptime, u64 idle_nsec, struct kernel_cpustat kcs

- Idle time source: get_idle_time

- Definition location: arch/x86/include/asm/current.h:28

- Mechanism: It uses architecture-specific per-cpu variables (accessed via the GS segment register on x86) to efficiently retrieve the pointer to the task_struct of the currently running process
