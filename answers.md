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


**Data table (ns/op):**
 

| N | List Ins | Hash Ins | RB Ins | XA Ins |
| | List Lkp | Hash Lkp | RB Lkp | XA Lkp |
|-------|----------|----------|--------|--------|
| 100 | | | | |
| 1000 | | | | |
| 5000 | | | | |
| 10000 | | | | |
| 50000 | | | | |

**Plot:** See bench_results.pdf

**Analysis (1 paragraph per operation):**

Insert: <your analysis>

Lookup: <your analysis>


1. [4 pts] GFP Flags and Context. What is the difference between GFP_KERNEL and GFP_ATOMIC? What
happens if you call kmalloc(size, GFP_KERNEL) while holding a spinlock? Why is this dangerous?
(Hint: think about what “may sleep” means for a CPU that must not be preempted.)
2. [4 pts] Safe Iteration. Why must you use list_for_each_entry_safe()instead of list_for_each_entry()
when deleting elements during iteration? Explain in terms of what happens to the next
pointer when you call list_del() on the current element.
3. [4 pts] Real-World Data Structure Usage. For each of the four data structures (linked list,
hash table, red-black tree, XArray), name one real Linux kernel subsystem that uses it and
explain why that data structure was chosen. (Hint: search the kernel source with grep -rn
"LIST_HEAD\|DEFINE_HASHTABLE\|rb_root\|DEFINE_XARRAY" –include="*.c" and pick interesting examples.)
4. [4 pts] Benchmark Analysis. From your Part B.3 results: which data structure shows O(1),
O(log n), and O(n) lookup behavior? At what dataset size does the linked list become impractical for lookups (i.e., >1 µs per lookup)? Show specific numbers from your measure-ments to support your answer.

5. [4 pts] Module Error Handling. (a) What happens if your module’s __init function returns a
non-zero value (e.g., -ENOMEM)? Does the kernel call your __exit function? (b) If proc_create() fails
but you ignore the error and continue loading, what problems will occur when the module
is later unloaded and calls proc_remove() with a NULL pointer? (c) Describe the correct error
handling pattern using goto cleanup labels (a common kernel coding idiom).


1 ## D: AI Usage Reflection
2
3 **Tools used:** <list all AI tools, or "none">
4
5 **How AI helped:**
6 <describe specific assistance with at least one concrete example>
7
8 **Limitations encountered:**
9 <describe incorrect/misleading AI responses, or "none encountered">
10
11 **Learning impact:**
12 <reflect on whether AI helped you learn vs. just complete tasks>
13
14 **Suggestions:**
15 Overall usefulness (1-5): <rating>
16 <suggestions for future exercise design>
