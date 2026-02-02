## 5.2 Kernel Macro Challenge

**container_of:**

- Definition location: include/linux/container_of.h:19

- One-sentence explanation: It calculates the memory address of a parent structure when given a pointer to one of its members.

- Usage in block/: block/blk-mq-sched.c:50

```
**block/blk-mq-sched.c:50:	struct request *rqa = container_of(a, struct request, queuelist);
```

- Explanation of usage: The variable a is a pointer to the queuelist member inside a struct request. To access the full struct request (which contains the actual I/O data), this macro subtracts the offset of queuelist from the address a.

**READ_ONCE / WRITE_ONCE:**

- Definition location: include/asm-generic/rwonce.h:47

- Why they exist: To prevent the compiler from performing optimizations like reordering or load tearing, ensuring that the variable is accessed exactly once as intended in concurrent programming


**current:**

- Definition location: arch/x86/include/asm/current.h:28

- Mechanism: It uses architecture-specific per-cpu variables (accessed via the GS segment register on x86) to efficiently retrieve the pointer to the task_struct of the currently running process
