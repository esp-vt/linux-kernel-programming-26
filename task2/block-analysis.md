## 3.1 Subsystem Statistics

**Commands used:**

- find block -name "*.c" | wc -l

- find block -name "*.c" -exec cat {} + | wc -l

- find block -name "*.c" -exec wc -l {} + | sort -nr | head -4

- grep -r "^config" block/Kconfig | wc -l

**Results:**

| Metric | Value |
|-----------------------|-------------|
| Total .c files | 74 |
| Total lines of C code | 65,298 |
| Kconfig options | 24 |

**3 largest .c files:**

1. block/bfq-iosched.c - 7691 lines

2. block/blk-mq.c - 5230 lines

3. block/blk-iocost.c - 3553 lines

## 3.2 Key Components

**struct request:**

- Location: include/linux/blk-mq.h:103

- Command used: grep -rn "struct request {"

**struct bio:**

- Location: include/linux/blk_types.h:210

- Command used: grep -rn "struct bio {"

**Relationship between bio and request:**

<2-3 sentences explaining the relationship based on code comments>

**Block device registration:**

- Function name: device_add_disk 

- Location: block/genhd.c:624

- Command used: grep -rn "device_add_disk"

## 3.3 Subsystem Interface

**Exported functions (5 examples):**

1. EXPORT_SYMBOL(badblocks_check); // from block/badblocks.c

2. EXPORT_SYMBOL_GPL(badblocks_set); // from block/badblocks.c

3. EXPORT_SYMBOL_GPL(badblocks_clear); // from block/badblocks.c

4. EXPORT_SYMBOL_GPL(ack_all_badblocks); // from block/badblocks.c
 
5. EXPORT_SYMBOL_GPL(badblocks_show); // from block/badblocks.c

**Commonly included headers:**

- <linux/blkdev.h>

- <linux/bio.h>

- <linux/module.h>

- <linux/kernel.h>

- <linux/slab.h>

**Data flow diagram:**

```
+-------------------------+
|    User Application     |
+------------+------------+
             | read() / write()
             v
+------------+------------+
|        VFS Layer        | (Virtual File System)
+------------+------------+
             | submit_bio()
             v
+------------+------------+
|    Block Layer Core     | <--- struct bio (Logical I/O)
+------------+------------+
             |
             v
+------------+------------+
|      I/O Scheduler      | (Merging & Sorting)
+------------+------------+
             |
             v
+------------+------------+
|   Block Device Driver   | <--- struct request (Physical I/O)
|     (NVMe, SCSI...)     |
+------------+------------+
             |
             v
+------------+------------+
|        Hardware         | (Disk / SSD)
+-------------------------+
```
