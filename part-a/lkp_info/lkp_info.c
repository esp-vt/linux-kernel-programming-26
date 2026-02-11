#define pr_fmt(fmt) "lkp: " fmt
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/jiffies.h>
#include <linux/atomic.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("LKP: System Info via /proc");

static unsigned long load_time_jiffies; 
static atomic_t access_count = ATOMIC_INIT(0); 

/* /proc/lkp_info 파일을 읽을 때 호출되는 함수 */
static int lkp_info_show(struct seq_file *m, void *v)
{
    unsigned long current_jiffies = jiffies;
    unsigned long elapsed_jiffies = current_jiffies - load_time_jiffies;
    
    /* 읽기 횟수 원자적 증가 */
    atomic_inc(&access_count); 

    seq_printf(m, "LKP Info Module\n");
    seq_printf(m, "Load time (jiffies): %lu\n", load_time_jiffies);
    seq_printf(m, "Current jiffies:     %lu\n", current_jiffies);
    seq_printf(m, "Uptime since load:   %lu jiffies (%u ms)\n", 
               elapsed_jiffies, jiffies_to_msecs(elapsed_jiffies));
    seq_printf(m, "Access count:        %d\n", atomic_read(&access_count));

    return 0;
}

static int lkp_info_open(struct inode *inode, struct file *file)
{
    return single_open(file, lkp_info_show, NULL);
}

/* 커널 5.6+ 버전에서 사용하는 proc_ops 구조체 */
static const struct proc_ops lkp_info_ops = {
    .proc_open    = lkp_info_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init lkp_info_init(void)
{
    load_time_jiffies = jiffies; 
    
    /* /proc/lkp_info 생성 */
    if (!proc_create("lkp_info", 0, NULL, &lkp_info_ops)) {
        pr_err("Failed to create /proc/lkp_info\n");
        return -ENOMEM;
    }
    
    pr_info("module loaded\n");
    return 0;
}

static void __exit lkp_info_exit(void)
{
    /* /proc/lkp_info 제거 */
    remove_proc_entry("lkp_info", NULL);
    pr_info("module unloaded\n");
}

module_init(lkp_info_init);
module_exit(lkp_info_exit);
