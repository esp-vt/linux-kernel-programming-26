#define pr_fmt(fmt) "lkp: " fmt
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("LKP: Hello World Module with Parameters");

/* 모듈 매개변수: 이름(name)과 횟수(count) */
static char *name = "LKP";
static int count = 1;

/* 매개변수 선언 및 권한 설정 (0644: 읽기 가능) */
module_param(name, charp, 0644);
MODULE_PARM_DESC(name, "The name to greet");
module_param(count, int, 0644);
MODULE_PARM_DESC(count, "How many times to greet");

static int __init lkp_hello_init(void)
{
    int i;
    pr_info("module loaded\n");

    /* 지정된 이름으로 count 횟수만큼 인사말 출력 */
    for (i = 0; i < count; i++) {
        pr_info("Hello, %s! (%d)\n", name, i + 1);
    }

    return 0;
}

static void __exit lkp_hello_exit(void)
{
    pr_info("Goodbye, %s! module unloaded\n", name);
}

module_init(lkp_hello_init);
module_exit(lkp_hello_exit);
