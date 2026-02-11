#define pr_fmt(fmt) "lkp: " fmt [cite: 68]
#include <linux/module.h> [cite: 69]
#include <linux/kernel.h> [cite: 70]
#include <linux/init.h> [cite: 71]

MODULE_LICENSE("GPL"); [cite: 72]
MODULE_AUTHOR("Your Name"); [cite: 73]
MODULE_DESCRIPTION("LKP: Hello World Module with Parameters"); [cite: 74]

/* 모듈 매개변수 선언 [cite: 104, 105] */
static char *name = "LKP"; [cite: 104]
static int count = 1; [cite: 105]

/* 매개변수 등록 및 설명 [cite: 124, 125] */
module_param(name, charp, 0644); [cite: 124, 126]
MODULE_PARM_DESC(name, "The name to greet"); [cite: 125]
module_param(count, int, 0644); [cite: 124, 126]
MODULE_PARM_DESC(count, "How many times to greet"); [cite: 125]

static int __init lkp_hello_init(void) [cite: 77]
{
    int i;
    pr_info("module loaded\n"); [cite: 78]

    /* count 횟수만큼 이름과 함께 인사말 출력 [cite: 106] */
    for (i = 0; i < count; i++) {
        pr_info("Hello, %s! (%d)\n", name, i + 1);
    }

    return 0; [cite: 78]
}

static void __exit lkp_hello_exit(void) [cite: 79]
{
    pr_info("Goodbye, %s! module unloaded\n", name); [cite: 85, 103]
}

module_init(lkp_hello_init); [cite: 87]
module_exit(lkp_hello_exit); [cite: 88]
