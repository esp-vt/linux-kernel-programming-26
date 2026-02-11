#define pr_fmt(fmt) "lkp: " fmt
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/hashtable.h>
#include <linux/rbtree.h>
#include <linux/xarray.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/ktime.h>
#include <linux/random.h>
#include <linux/string.h>
#include <linux/compiler.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");

struct my_entry {
    int value;
    int index;
    struct list_head node;
    struct hlist_node hnode;
    struct rb_node rb_node;
};

/* 전역 변수 및 파라미터 */
static char *int_str = "";
module_param(int_str, charp, 0644);
static int bench_size = 1000;
module_param(bench_size, int, 0644);

static volatile int dummy_sum = 0;

static LIST_HEAD(my_list);
static DEFINE_HASHTABLE(my_hash, 14);
static struct rb_root my_rb = RB_ROOT;
static DEFINE_XARRAY(my_xa);

static u64 ins_results[4], lkp_results[4];

/* 1. 프로토타입 경고 해결: static 선언 */
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

/* 출력 함수들 */
static int lkp_ds_show(struct seq_file *m, void *v) {
    struct my_entry *entry;
    int bkt;
    unsigned long index;
    bool first = true;

    seq_printf(m, "Linked list:   ");
    list_for_each_entry(entry, &my_list, node) {
        if (entry->index < 100000) {
            seq_printf(m, "%s%d", first ? "" : ", ", entry->value);
            first = false;
        }
    }
    seq_printf(m, "\nHash table:    ");
    first = true;
    hash_for_each(my_hash, bkt, entry, hnode) {
        if (entry->index < 100000) {
            seq_printf(m, "%s%d", first ? "" : ", ", entry->value);
            first = false;
        }
    }
    seq_printf(m, "\nRed-black tree: ");
    first = true;
    for (struct rb_node *n = rb_first(&my_rb); n; n = rb_next(n)) {
        entry = rb_entry(n, struct my_entry, rb_node);
        if (entry->index < 100000) {
            seq_printf(m, "%s%d", first ? "" : ", ", entry->value);
            first = false;
        }
    }
    seq_printf(m, " (sorted)\nXArray:         ");
    first = true;
    xa_for_each(&my_xa, index, entry) {
        if (entry->index < 100000) {
            seq_printf(m, "%s%d", first ? "" : ", ", entry->value);
            first = false;
        }
    }
    seq_printf(m, " (by index)\n");
    return 0;
}

static int lkp_ds_bench_show(struct seq_file *m, void *v) {
    seq_printf(m, "LKP Data Structure Benchmark (N=%d)\n", bench_size);
    seq_printf(m, "=======================================\n");
    seq_printf(m, "Insert (ns/op):\nLinked list: %llu\nHash table: %llu\nRed-black tree: %llu\nXArray: %llu\n\n",
               ins_results[0], ins_results[1], ins_results[2], ins_results[3]);
    seq_printf(m, "Lookup (ns/op):\nLinked list: %llu\nHash table: %llu\nRed-black tree: %llu\nXArray: %llu\n",
               lkp_results[0], lkp_results[1], lkp_results[2], lkp_results[3]);
    return 0;
}

/* 2. 에러 해결: lkp_ds_bench_open 내에서 lkp_ds_bench_show를 호출해야 함 */
static int lkp_ds_open(struct inode *i, struct file *f) { 
    return single_open(f, lkp_ds_show, NULL); 
}
static int lkp_ds_bench_open(struct inode *i, struct file *f) { 
    return single_open(f, lkp_ds_bench_show, NULL); 
}

static const struct proc_ops ds_ops = { .proc_open = lkp_ds_open, .proc_read = seq_read, .proc_release = single_release };
static const struct proc_ops bench_ops = { .proc_open = lkp_ds_bench_open, .proc_read = seq_read, .proc_release = single_release };

static int __init lkp_ds_init(void) {
    char *s, *p, *working_str;
    int val, i;
    struct my_entry *e, *se;
    ktime_t start;
    int *vals;

    if (int_str && *int_str) {
        working_str = kstrdup(int_str, GFP_KERNEL);
        p = working_str; i = 0;
        while ((s = strsep(&p, ",")) != NULL) {
            if (kstrtoint(s, 10, &val) == 0) {
                e = kmalloc(sizeof(*e), GFP_KERNEL);
                if (!e) continue;
                e->value = val; e->index = i++;
                list_add_tail(&e->node, &my_list);
                hash_add(my_hash, &e->hnode, val);
                insert_rbtree(&my_rb, e);
                xa_store(&my_xa, e->index, e, GFP_KERNEL);
            }
        }
        kfree(working_str);
    }

    vals = kmalloc_array(bench_size, sizeof(int), GFP_KERNEL);
    if (!vals) return -ENOMEM;
    for (i = 0; i < bench_size; i++) vals[i] = get_random_u32();

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
            barrier(); // 3. 에러 해결: 최신 커널은 optimization_barrier 대신 barrier() 사용
        }
        ins_results[ds] = ktime_to_ns(ktime_sub(ktime_get(), start)) / bench_size;
    }

    /* Lookup Benchmarks... (barrier() 사용 준수) */
    start = ktime_get();
    for (i = 0; i < (bench_size > 1000 ? 100 : 1000); i++) {
        int target = vals[bench_size - 1];
        list_for_each_entry(se, &my_list, node) {
            if (se->value == target) { dummy_sum += se->value; break; }
            barrier();
        }
    }
    lkp_results[0] = ktime_to_ns(ktime_sub(ktime_get(), start)) / i;

    // Hash Lookup
    start = ktime_get();
    for (i = 0; i < bench_size; i++) {
        hash_for_each_possible(my_hash, se, hnode, vals[i]) {
            if (se->value == vals[i]) { dummy_sum += se->value; break; }
        }
        barrier();
    }
    lkp_results[1] = ktime_to_ns(ktime_sub(ktime_get(), start)) / bench_size;

    // RB-Tree Lookup
    start = ktime_get();
    for (i = 0; i < bench_size; i++) {
        struct rb_node *n = my_rb.rb_node;
        while (n) {
            se = rb_entry(n, struct my_entry, rb_node);
            if (vals[i] < se->value) n = n->rb_left;
            else if (vals[i] > se->value) n = n->rb_right;
            else { dummy_sum += se->value; break; }
        }
        barrier();
    }
    lkp_results[2] = ktime_to_ns(ktime_sub(ktime_get(), start)) / bench_size;

    // XArray Lookup
    start = ktime_get();
    for (i = 0; i < bench_size; i++) {
        se = xa_load(&my_xa, i + 100000);
        if (se) dummy_sum += se->value;
        barrier();
    }
    lkp_results[3] = ktime_to_ns(ktime_sub(ktime_get(), start)) / bench_size;

    kfree(vals);
    proc_create("lkp_ds", 0, NULL, &ds_ops);
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
    remove_proc_entry("lkp_ds", NULL);
    remove_proc_entry("lkp_ds_bench", NULL);
}

module_init(lkp_ds_init);
module_exit(lkp_ds_exit);
