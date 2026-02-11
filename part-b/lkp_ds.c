#define pr_fmt(fmt) "lkp: " fmt
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>      /* Linked List [cite: 30] */
#include <linux/hashtable.h> /* Hash Table [cite: 30] */
#include <linux/rbtree.h>    /* Red-Black Tree [cite: 30] */
#include <linux/xarray.h>    /* XArray [cite: 30] */
#include <linux/slab.h>      /* kmalloc [cite: 231] */
#include <linux/proc_fs.h>   /* /proc [cite: 195] */
#include <linux/seq_file.h>  /* seq_file [cite: 196] */
#include <linux/ktime.h>     /* ktime_get_ns [cite: 334] */
#include <linux/random.h>    /* get_random_u32 [cite: 348] */
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("LKP: Data Structures Scalability Benchmark");

struct my_entry {
    int value;
    int index;
    struct list_head node;
    struct hlist_node hnode;
    struct rb_node rb_node;
};

/* Parameters [cite: 244, 352] */
static char *int_str = "";
module_param(int_str, charp, 0644);
static int bench_size = 1000;
module_param(bench_size, int, 0644);

/* Data Structures [cite: 259] */
static LIST_HEAD(my_list);
static DEFINE_HASHTABLE(my_hash, 14); /* 2^14 buckets for scale [cite: 475] */
static struct rb_root my_rb = RB_ROOT;
static DEFINE_XARRAY(my_xa);

/* Benchmark Results (nanoseconds per operation) [cite: 358] */
static u64 ins_results[4], lkp_results[4];

/* RB-Tree Insertion [cite: 305, 307] */
void insert_rbtree(struct rb_root *root, struct my_entry *data) {
    struct rb_node **new = &(root->rb_node), *parent = NULL;
    while (*new) {
        struct my_entry *this = rb_entry(*new, struct my_entry, rb_node);
        parent = *new;
        if (data->value < this->value) new = &((*new)->rb_left);
        else new = &((*new)->rb_right);
    }
    rb_link_node(&data->rb_node, parent, new);
    rb_insert_color(&data->rb_node, root);
}

/* Part B.1 & B.2 Display (/proc/lkp_ds) [cite: 249, 284] */
static int lkp_ds_show(struct seq_file *m, void *v) {
    struct my_entry *entry;
    int bkt;
    unsigned long index;
    bool first = true;

    seq_printf(m, "Linked list:   ");
    list_for_each_entry(entry, &my_list, node) {
        seq_printf(m, "%s%d", first ? "" : ", ", entry->value);
        first = false;
    }
    
    seq_printf(m, "\nHash table:    ");
    first = true;
    hash_for_each(my_hash, bkt, entry, hnode) {
        seq_printf(m, "%s%d", first ? "" : ", ", entry->value);
        first = false;
    }

    seq_printf(m, "\nRed-black tree: ");
    first = true;
    for (struct rb_node *n = rb_first(&my_rb); n; n = rb_next(n)) {
        seq_printf(m, "%s%d", first ? "" : ", ", rb_entry(n, struct my_entry, rb_node)->value);
        first = false;
    }
    seq_printf(m, " (sorted)");

    seq_printf(m, "\nXArray:         ");
    first = true;
    xa_for_each(&my_xa, index, entry) {
        seq_printf(m, "%s%d", first ? "" : ", ", entry->value);
        first = false;
    }
    seq_printf(m, " (by index)\n");
    return 0;
}

/* Part B.3 Benchmark Report (/proc/lkp_ds_bench) [cite: 358-360] */
static int lkp_ds_bench_show(struct seq_file *m, void *v) {
    seq_printf(m, "LKP Data Structure Benchmark (N=%d)\n", bench_size);
    seq_printf(m, "=======================================\n");
    seq_printf(m, "Insert (ns/op):\n");
    seq_printf(m, "Linked list: %llu\nHash table: %llu\nRed-black tree: %llu\nXArray: %llu\n\n",
               ins_results[0], ins_results[1], ins_results[2], ins_results[3]);
    seq_printf(m, "Lookup (ns/op):\n");
    seq_printf(m, "Linked list: %llu\nHash table: %llu\nRed-black tree: %llu\nXArray: %llu\n",
               lkp_results[0], lkp_results[1], lkp_results[2], lkp_results[3]);
    return 0;
}

static int lkp_ds_open(struct inode *i, struct file *f) { return single_open(f, lkp_ds_show, NULL); }
static int lkp_ds_bench_open(struct inode *i, struct file *f) { return single_open(f, lkp_ds_bench_show, NULL); }

static const struct proc_ops ds_ops = { .proc_open = lkp_ds_open, .proc_read = seq_read, .proc_release = single_release };
static const struct proc_ops bench_ops = { .proc_open = lkp_ds_bench_open, .proc_read = seq_read, .proc_release = single_release };

static int __init lkp_ds_init(void) {
    char *s, *p, *working_str;
    int val, i;
    struct my_entry *entry, *search_entry;
    u64 start;
    int *bench_values;

    /* 1. Correctness Data (int_str) [cite: 243-248] */
    if (int_str && *int_str) {
        working_str = kstrdup(int_str, GFP_KERNEL);
        p = working_str;
        i = 0;
        while ((s = strsep(&p, ",")) != NULL) {
            if (kstrtoint(s, 10, &val) == 0) {
                entry = kmalloc(sizeof(*entry), GFP_KERNEL);
                if (!entry) continue;
                entry->value = val;
                entry->index = i++;
                list_add_tail(&entry->node, &my_list);
                hash_add(my_hash, &entry->hnode, val);
                insert_rbtree(&my_rb, entry);
                xa_store(&my_xa, entry->index, entry, GFP_KERNEL);
            }
        }
        kfree(working_str);
    }

    /* 2. Scalability Benchmark [cite: 351-357] */
    bench_values = kmalloc_array(bench_size, sizeof(int), GFP_KERNEL);
    if (!bench_values) return -ENOMEM;

    for (i = 0; i < bench_size; i++) bench_values[i] = get_random_u32();

    /* Insert Benchmark */
    for (int ds = 0; ds < 4; ds++) {
        start = ktime_get_ns();
        for (i = 0; i < bench_size; i++) {
            entry = kmalloc(sizeof(*entry), GFP_KERNEL);
            if (!entry) break;
            entry->value = bench_values[i];
            entry->index = i + 1000000; /* Distinct from correctness data [cite: 474] */
            if (ds == 0) list_add_tail(&entry->node, &my_list);
            else if (ds == 1) hash_add(my_hash, &entry->hnode, entry->value);
            else if (ds == 2) insert_rbtree(&my_rb, entry);
            else xa_store(&my_xa, entry->index, entry, GFP_KERNEL);
        }
        ins_results[ds] = (ktime_get_ns() - start) / bench_size;
    }

    /* Lookup Benchmark [cite: 356] */
    /* List Lookup (O(n)) */
    start = ktime_get_ns();
    for (i = 0; i < (bench_size > 1000 ? 1000 : bench_size); i++) { /* Limit list search to save time */
        list_for_each_entry(search_entry, &my_list, node) {
            if (search_entry->value == bench_values[i]) break;
        }
    }
    lkp_results[0] = (ktime_get_ns() - start) / (i);

    /* Hash Lookup (O(1)) */
    start = ktime_get_ns();
    for (i = 0; i < bench_size; i++) {
        hash_for_each_possible(my_hash, search_entry, hnode, bench_values[i]) {
            if (search_entry->value == bench_values[i]) break;
        }
    }
    lkp_results[1] = (ktime_get_ns() - start) / bench_size;

    /* RB-Tree Lookup (O(log n)) */
    start = ktime_get_ns();
    for (i = 0; i < bench_size; i++) {
        struct rb_node *node = my_rb.rb_node;
        while (node) {
            search_entry = rb_entry(node, struct my_entry, rb_node);
            if (bench_values[i] < search_entry->value) node = node->rb_left;
            else if (bench_values[i] > search_entry->value) node = node->rb_right;
            else break;
        }
    }
    lkp_results[2] = (ktime_get_ns() - start) / bench_size;

    /* XArray Lookup (O(1)) */
    start = ktime_get_ns();
    for (i = 0; i < bench_size; i++) {
        xa_load(&my_xa, i + 1000000);
    }
    lkp_results[3] = (ktime_get_ns() - start) / bench_size;

    kfree(bench_values);
    proc_create("lkp_ds", 0, NULL, &ds_ops);
    proc_create("lkp_ds_bench", 0, NULL, &bench_ops);
    return 0;
}

static void __exit lkp_ds_exit(void) {
    struct my_entry *entry, *tmp;
    list_for_each_entry_safe(entry, tmp, &my_list, node) {
        list_del(&entry->node);
        kfree(entry); /* [cite: 235, 621] */
    }
    xa_destroy(&my_xa); /* [cite: 310] */
    remove_proc_entry("lkp_ds", NULL);
    remove_proc_entry("lkp_ds_bench", NULL);
}

module_init(lkp_ds_init);
module_exit(lkp_ds_exit);
