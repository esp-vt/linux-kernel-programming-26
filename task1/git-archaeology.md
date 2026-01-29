## 2.1 Commit Investigation

**Q1: Commit count for kernel/sched/ (v6.16..v6.18)**

- Command used: git log --oneline v6.16..v6.18 kernel/sched/ | wc -l

- Answer: 179 commits

**Q2: Latency-related commit**

- Command used: git log --oneline --grep="latency" v6.16..v6.18 kernel/sched/

- Commit: <052c3d87c82e> <sched/fair: Limit run to parity to the min slice of enqueued entities>

- Problem addressed: <1-2 sentence explanation>

**Q3: Top 3 contributors**

- Command used: git shortlog -sn v6.16..v6.18 kernel/sched/ | head -5

- Results:

1. Ingo Molnar - 42 commits

2. Tejun Heo - 31 commits

3. Linux Torvalds - 17 commits

## 2.2 Blame Analysis

**Command used:** git blame -L 200,250 mm/mmap.c

**Oldest line identified:**

- Line number: 213-214

- Commit hash: 1da177e4c3f4

- Commit date: 2005-04-16

- Author: Linux Torvalds

- Kernel version: v2.6.12-rc2 (or "initial git import")

**How I identified this:** 

I simply compare the date of each commit and figured out the oldest commit. And
I used `git show --oneline 1da177e4c3f4` to see the kernel version of the hash I found.

## 2.3 Merge Commit Analysis

**Merge commit found:**

- Command used:

`git log --merges -1`

`git log --no-merges -1`

- Commit hash: e69c7c175115c51c7f95394fc55425a395b3af59

- Message summary: <Merge tag 'timers_urgent_for_v6.18_rc8' of `git://git.kernel.org/pub/scm/linux/kernel/git/tip/tip`- timekeeping: Fix error code in tk_aux_sysfs_init()>

**Comparison (merge vs regular commit):**

| Aspect | Merge Commit | Regular Commit |
|-----------------|-------------------|-------------------|
| Parent count | 2 (Merge: 6bda50f4333f c7418164b463) | 1 |
| Merge line | Present | empty |
| Stats scope | Integrates external work (Pull Request) | single change by an user |

**What merge messages reveal about workflow:**

Merge request says the third engineers can develop or fix the code logic inside of linux. As I observed the error fixing on the function `tk_aux_sysfs_init()`, the linux code environment is open-sourced and anyone interested in the code can contribute here. In the end, the head of the repo who is Linus Torvalds will admit such changes.
