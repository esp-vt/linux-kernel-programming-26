## A.1 Hello LKP Module

**Files created:**

- lkp_hello.c

- Makefile

**Test output (dmesg | grep "lkp:"):**
 
[ 1510.848055] lkp: module loaded
[ 1510.848056] lkp: Hello, LKP! (1)
[ 1510.885386] lkp: Goodbye, LKP! module unloaded

**Parameter verification:**

$ cat /sys/module/lkp_hello/parameters/name

LKP

$ cat /sys/module/lkp_hello/parameters/count

1



## A.2 Module with /proc Interface

**Files created:**

- lkp_info.c

- Makefile


**Test output:**

$ cat /proc/lkp_info

```
LKP Info Module
Load time (jiffies): 4296418593
Current jiffies:     4296424022
Uptime since load:   5429 jiffies (5429 ms)
Access count:        1
```

$ cat /proc/lkp_info # second read

```
KP Info Module
Load time (jiffies): 4296418593
Current jiffies:     4296425514
Uptime since load:   6921 jiffies (6921 ms)
Access count:        2
```

## B.1 Linked List & Hash Table

**Test output:**

$ cat /proc/lkp_ds

```
Linked list: 1, 2, 3, 4, 5
Hash table:  3, 1, 4, 2, 5
```

**Memory cleanup verification:**
 
$ sudo rmmod lkp_ds

$ dmesg | grep "lkp:" 

```
[ 2403.701524] lkp: module loaded
[ 2475.395015] lkp: freed entry
[ 2475.395027] lkp: freed entry
[ 2475.395030] lkp: freed entry
[ 2475.395032] lkp: freed entry
[ 2475.395033] lkp: freed entry
[ 2475.395039] lkp: module unloaded
```

## B.2 Red-Black Tree & XArray

**Test output:**

$ cat /proc/lkp_ds

```
Linked list:   1, 2, 3, 4, 5
Hash table:    3, 1, 4, 2, 5
Red-black tree: 1, 2, 3, 4, 5 (sorted)
XArray:         1, 2, 3, 4, 5 (by index)
```

**Verification with different input:**

$ sudo rmmod lkp_ds

$ sudo insmod lkp_ds.ko int_str="5,3,1,4,2"

$ cat /proc/lkp_ds

```
Linked list:   5, 3, 1, 4, 2
Hash table:    3, 1, 4, 2, 5
Red-black tree: 1, 2, 3, 4, 5 (sorted)
XArray:         5, 3, 1, 4, 2 (by index)
```

## B.3 Scalability Benchmark

**Benchmark output for N=10000:**

$ cat /proc/lkp_ds_bench

LKP Data Structure Benchmark (N=10000)
=======================================
Insert (ns/op):
  Linked list:    40
  Hash table:     50
  Red-black tree: 184
  XArray:         82

Lookup (ns/op):
  Linked list:    50730
  Hash table:     61
  Red-black tree: 198
  XArray:         74

**Data table (ns/op):**
 

| N | List Ins | Hash Ins | RB Ins | XA Ins |
| | List Lkp | Hash Lkp | RB Lkp | XA Lkp |
|-------|----------|----------|--------|--------|
| 100 | 178 | 95 | 310 | 280 |
|      | 658 | 11 | 229 | 350 |
| 1000 | 112 | 59 | 230 | 145 |
|      | 8168 | 2 | 52 | 14 |
| 5000 | 53 55 215 115 | 
|       | 39191 9 69 22 |
| 10000 | 27 | 43 | 123 | 72 |
|       | 53353 | 15 | 111 | 42 |
| 50000 | 46 | 45 | 177 | 60 |
|       | 1799457 | 46 | 203 | 44 |

**Plot:** See bench_results.pdf

**Analysis (1 paragraph per operation):**

Insert: For the insertion operation, the time taken per operation remains relatively stable across all data structures as the dataset size ($N$) increases, indicating that the overhead of memory allocation (kmalloc) is a dominant factor at smaller scales. While Linked List and Hash Table showed the fastest insertion times due to their simple append or bucket-mapping logic, the Red-Black Tree consistently took longer (ranging from 310 ns to 177 ns) because it requires additional CPU cycles to rebalance the tree nodes to maintain a sorted property. Overall, the insertion performance for all four structures remains within a practical nanosecond range, proving that the kernel's memory management and these algorithms handle high-frequency data entry efficiently.

Lookup: The lookup operation clearly illustrates the theoretical time complexity of each data structure. The Linked List demonstrates a classic $O(n)$ behavior, where the lookup time explodes from 658 ns at $N=100$ to nearly 1.8 ms (1,799,457 ns) at $N=50,000$, making it completely impractical for large datasets. In contrast, the Hash Table and XArray exhibit $O(1)$ behavior, maintaining nearly constant and extremely low lookup times (often under 50 ns) regardless of $N$. The Red-Black Tree follows an $O(\log n)$ trend, showing a very gradual increase in time as the tree depth grows, yet remaining significantly faster than the linked list at higher scales. Specifically, the linked list becomes a performance bottleneck once $N$ exceeds 1,000, where its lookup time reaches 8,168 ns, far exceeding the 1 $\mu$s threshold.


1. GFP Flags and Context. What is the difference between GFP_KERNEL and GFP_ATOMIC? What happens if you call kmalloc(size, GFP_KERNEL) while holding a spinlock? Why is this dangerous? (Hint: think about what “may sleep” means for a CPU that must not be preempted.)

Holding a spinlock while calling a blocking allocation like GFP_KERNEL is dangerous because it can cause the CPU to sleep while holding a lock, leading to a permanent system deadlock where no other task can acquire that lock.

2. Safe Iteration. Why must you use list_for_each_entry_safe() instead of list_for_each_entry() when deleting elements during iteration? Explain in terms of what happens to the next pointer when you call list_del() on the current element.

`list_for_each_entry_safe()` uses a temporary pointer to store the next element before any deletion occurs, ensuring the iterator can still find the following node even after the current node's pointers are modified by list_del().

3. Real-World Data Structure Usage. For each of the four data structures (linked list, hash table, red-black tree, XArray), name one real Linux kernel subsystem that uses it and explain why that data structure was chosen. (Hint: search the kernel source with grep -rn "LIST_HEAD\|DEFINE_HASHTABLE\|rb_root\|DEFINE_XARRAY" --include="*.c" and pick interesting examples.)

Linked List: 
kernel/kprobes.c:513:static LIST_HEAD(optimizing_list);

In kernel/kprobes.c, a linked list is used to queue kprobes that are ready for optimization, allowing the kernel to process them sequentially.

Table: 
kernel/workqueue.c:1314:static DEFINE_HASHTABLE(wci_hash, ilog2(WCI_MAX_ENTS));

In kernel/workqueue.c, the workqueue subsystem uses a hash table to efficiently map and manage workqueue completion items for fast lookup based on specific keys.

Tree: 
arch/x86/kernel/cpu/sgx/main.c:27:static DEFINE_XARRAY(sgx_epc_address_space);

DEFINE_XARRAY for SGX, a specific SGX XArray tracks the EPC (Enclave Page Cache) address space for efficient page management.

XArray: 
arch/arm64/mm/mteswap.c:10:static DEFINE_XARRAY(mte_pages);

XArray here is used to store and retrieve Memory Tagging Extension (MTE) pages based on their index, providing a memory-efficient alternative to sparse arrays for swap operations.

4. Benchmark Analysis. From your Part B.3 results: which data structure shows O(1), O(log n), and O(n) lookup behavior? At what dataset size does the linked list become impractical for lookups (i.e., >1 us per lookup)? Show specific numbers from your measurements to support your answer.

Measurements show $O(1)$ for Hash/XArray, $O(\log n)$ for RB-Tree, and $O(n)$ for Linked List, with the list becoming impractical at $N=1,000$ where lookup time reached 8,168 ns ($8.16 \mu s$), far exceeding the $1 \mu s$ threshold.

5. Module Error Handling. (a) What happens if your module’s __init function returns a non-zero value (e.g., -ENOMEM)? Does the kernel call your __exit function? (b) If proc_create() fails but you ignore the error and continue loading, what problems will occur when the module is later unloaded and calls proc_remove() with a NULL pointer? (c) Describe the correct error handling pattern using goto cleanup labels (a common kernel coding idiom).

(a) If __init returns a non-zero value, the module fails to load and the kernel does not call the __exit function.

(b) Ignoring a proc_create() failure leads to a kernel panic or system crash when proc_remove() is later called with a NULL pointer during unloading.

(c) The standard kernel idiom involves using goto labels to release previously allocated resources in reverse order upon encountering an error.

## D: AI Usage Reflection

**Tools used:** Gemini

**How AI helped:**

It helped me correcting a grammar issue on my writing, and understanding of the assinment's requirements.

**Limitations encountered:**

none encountered

**Learning impact:**

 It is helpful for doing the assignment. 
 
**Suggestions:**
Overall usefulness (1-5): 5

N/A

