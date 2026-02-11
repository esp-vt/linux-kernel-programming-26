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

static unsigned long load_time_jiffies; // 모듈 로드 시점 저장 
static atomic_t access_count = ATOMIC_INIT(0); // 읽기 횟수 (원자적 연산) [cite: 151, 153]

/* /proc 파일을 읽을 때 호출되는 함수 [cite: 200] */
static int lkp_info_show(struct seq_file *m, void *v)
{
    unsigned long current_jiffies = jiffies; [cite: 171]
    unsigned long elapsed_jiffies = current_jiffies - load_time_jiffies; [cite: 172]
    
    atomic_inc(&access_count); // 읽을 때마다 카운트 증가 [cite: 152, 173]

    seq_printf(m, "LKP Info Module\n"); [cite: 177, 196]
    seq_printf(m, "Load time (jiffies): %lu\n", load_time_jiffies); [cite: 179]
    seq_printf(m, "Current jiffies:     %lu\n", current_jiffies); [cite: 180]
    seq_printf(m, "Uptime since load:   %lu jiffies (%u ms)\n", 
               elapsed_jiffies, jiffies_to_msecs(elapsed_jiffies)); [cite: 182, 198]
    seq_printf(m, "Access count:        %d\n", atomic_read(&access_count)); [cite: 154, 183]

    return 0;
}

static int lkp_info_open(struct inode *inode, struct file *file)
{
    return single_open(file, lkp_info_show, NULL);
}

/* /proc 파일 오퍼레이션 설정 */
static const struct proc_ops lkp_info_ops = {
    .proc_open    = lkp_info_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init lkp_info_init(void)
{
    load_time_jiffies = jiffies; // 로드 시점 기록 
    /* /proc/lkp_info 생성 [cite: 146, 169, 195] */
    if (!proc_create("lkp_info", 0, NULL, &lkp_info_ops)) {
        return -ENOMEM;
    }
    pr_info("module loaded\n");
    return 0;
}

static void __exit lkp_info_exit(void)
{
    proc_remove(proc_lookup(NULL, "lkp_info")); // /proc 항목 제거 [cite: 174, 195]
    pr_info("module unloaded\n");
}

module_init(lkp_info_init);
module_exit(lkp_info_exit);
