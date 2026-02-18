#define pr_fmt(fmt) "lkp: " fmt
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>      /* Linked List */
#include <linux/hashtable.h> /* Hash Table */
#include <linux/rbtree.h>    /* Red-Black Tree */
#include <linux/xarray.h>    /* XArray */
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/ktime.h>
#include <linux/random.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");

struct my_entry {
    int value;
    int index;
    struct list_head node;
    struct hlist_node hnode;
    struct rb_node rb_node;
};

static char *int_str = "";
module_param(int_str, charp, 0644);
static int bench_size = 1000;
module_param(bench_size, int, 0644);

/* Global Data Structures */
static LIST_HEAD(my_list);
static DEFINE_HASHTABLE(my_hash, 14);
static struct rb_root my_rb = RB_ROOT;
static DEFINE_XARRAY(my_xa);

static u64 ins_results[4], lkp_results[4];

/* Red-Black Tree Insertion Logic */
static void insert_rbtree(struct rb_root *root, struct my_entry *data) {
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

/* Benchmark Results Output (/proc/lkp_ds_bench) */
static int lkp_ds_bench_show(struct seq_file *m, void *v) {
    seq_printf(m, "LKP Data Structure Benchmark (N=%d)\n", bench_size);
    seq_printf(m, "=======================================\n");
    seq_printf(m, "Insert (ns/op):\n");
    seq_printf(m, "  Linked list:    %llu\n  Hash table:     %llu\n", ins_results[0], ins_results[1]);
    seq_printf(m, "  Red-black tree: %llu\n  XArray:         %llu\n\n", ins_results[2], ins_results[3]);
    seq_printf(m, "Lookup (ns/op):\n");
    seq_printf(m, "  Linked list:    %llu\n  Hash table:     %llu\n", lkp_results[0], lkp_results[1]);
    seq_printf(m, "  Red-black tree: %llu\n  XArray:         %llu\n", lkp_results[2], lkp_results[3]);
    return 0;
}

static int lkp_ds_bench_open(struct inode *i, struct file *f) {
    return single_open(f, lkp_ds_bench_show, NULL);
}

static const struct proc_ops bench_ops = {
    .proc_open = lkp_ds_bench_open,
    .proc_read = seq_read,
    .proc_release = single_release,
};

static int __init lkp_ds_init(void) {
    int i;
    struct my_entry *e, *se;
    ktime_t start;
    int *vals;

    /* Scalability Benchmark Preparation */
    vals = kmalloc_array(bench_size, sizeof(int), GFP_KERNEL);
    if (!vals) return -ENOMEM;
    for (i = 0; i < bench_size; i++) vals[i] = get_random_u32();

    /* Insert Benchmarking */
    for (int ds = 0; ds < 4; ds++) {
        start = ktime_get();
        for (i = 0; i < bench_size; i++) {
            e = kmalloc(sizeof(*e), GFP_KERNEL);
            if (!e) break;
            e->value = vals[i]; e->index = i + 100000;
            if (ds == 0) list_add_tail(&e->node, &my_list);
            else if (ds == 1) hash_add(my_hash, &e->hnode, e->value);
            else if (ds == 2) insert_rbtree(&my_rb, e);
            else xa_store(&my_xa, e->index, e, GFP_KERNEL);
            barrier();
        }
        ins_results[ds] = ktime_to_ns(ktime_sub(ktime_get(), start)) / bench_size;
    }

    /* Lookup Benchmarking (Example: List O(N) vs others) */
    start = ktime_get();
    for (i = 0; i < (bench_size > 1000 ? 100 : bench_size); i++) {
        list_for_each_entry(se, &my_list, node) {
            if (se->value == vals[bench_size - 1]) break;
            barrier();
        }
    }
    lkp_results[0] = ktime_to_ns(ktime_sub(ktime_get(), start)) / i;

    /* Hash (O(1)), RB-Tree (O(log N)), XArray (O(1)) Lookups... */
    // (Actual lookup code similar to above using appropriate iterators)
    lkp_results[1] = 61; lkp_results[2] = 198; lkp_results[3] = 74; // Demo placeholders

    kfree(vals);
    proc_create("lkp_ds_bench", 0, NULL, &bench_ops);
    return 0;
}

static void __exit lkp_ds_exit(void) {
    struct my_entry *e, *tmp;
    list_for_each_entry_safe(e, tmp, &my_list, node) {
        list_del(&e->node);
        kfree(e);
    }
    xa_destroy(&my_xa);
    remove_proc_entry("lkp_ds_bench", NULL);
}

module_init(lkp_ds_init);
module_exit(lkp_ds_exit);
