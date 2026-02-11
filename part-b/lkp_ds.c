#define pr_fmt(fmt) "lkp: " fmt
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/hashtable.h>
#include <linux/rbtree.h>
#include <linux/xarray.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/ktime.h>
#include <linux/random.h>

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

static LIST_HEAD(my_list);
static DEFINE_HASHTABLE(my_hash, 10);
static struct rb_root my_rb = RB_ROOT;
static DEFINE_XARRAY(my_xa);

// RB-Tree 삽입 함수 [cite: 305]
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

// /proc/lkp_ds 출력 함수 [cite: 249, 284]
static int lkp_ds_show(struct seq_file *m, void *v) {
    struct my_entry *entry;
    int bkt;
    unsigned long index;

    seq_printf(m, "Linked list:\n");
    list_for_each_entry(entry, &my_list, node) seq_printf(m, "%d ", entry->value);
    
    seq_printf(m, "\nHash table:\n");
    hash_for_each(my_hash, bkt, entry, hnode) seq_printf(m, "%d ", entry->value);

    seq_printf(m, "\nRed-black tree:\n");
    for (struct rb_node *node = rb_first(&my_rb); node; node = rb_next(node))
        seq_printf(m, "%d ", rb_entry(node, struct my_entry, rb_node)->value);

    seq_printf(m, "\nXArray:\n");
    xa_for_each(&my_xa, index, entry) seq_printf(m, "%d ", entry->value);
    seq_printf(m, "\n");
    return 0;
}

static int lkp_ds_open(struct inode *inode, struct file *file) {
    return single_open(file, lkp_ds_show, NULL);
}

static const struct proc_ops ds_ops = { .proc_open = lkp_ds_open, .proc_read = seq_read, .proc_release = single_release };

static int __init lkp_ds_init(void) {
    // 문자열 파싱 및 데이터 삽입 로직 (생략된 로직 구현 필요)
    proc_create("lkp_ds", 0, NULL, &ds_ops);
    return 0;
}

static void __exit lkp_ds_exit(void) {
    struct my_entry *entry;
    struct list_head *pos, *q;
    list_for_each_safe(pos, q, &my_list) {
        entry = list_entry(pos, struct my_entry, node);
        list_del(pos);
        kfree(entry);
    }
    xa_destroy(&my_xa);
    remove_proc_entry("lkp_ds", NULL);
}

module_init(lkp_ds_init);
module_exit(lkp_ds_exit);
