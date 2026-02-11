#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const char ____versions[]
__used __section("__versions") =
	"\x10\x00\x00\x00\x24\x5c\xa2\x8f"
	"xa_find\0"
	"\x18\x00\x00\x00\x4b\x91\x36\xdf"
	"xa_find_after\0\0\0"
	"\x28\x00\x00\x00\xb3\x1c\xa2\x87"
	"__ubsan_handle_out_of_bounds\0\0\0\0"
	"\x1c\x00\x00\x00\xcb\xf6\xfd\xf0"
	"__stack_chk_fail\0\0\0\0"
	"\x10\x00\x00\x00\xba\x0c\x7a\x03"
	"kfree\0\0\0"
	"\x14\x00\x00\x00\x14\x1c\x49\xd9"
	"xa_destroy\0\0"
	"\x1c\x00\x00\x00\x25\x01\xcd\x63"
	"remove_proc_entry\0\0\0"
	"\x14\x00\x00\x00\x45\x3a\x23\xeb"
	"__kmalloc\0\0\0"
	"\x18\x00\x00\x00\x0c\xc1\x6d\xd3"
	"get_random_u32\0\0"
	"\x14\x00\x00\x00\x65\x93\x3f\xb4"
	"ktime_get\0\0\0"
	"\x14\x00\x00\x00\x2f\x28\x3e\x5b"
	"xa_store\0\0\0\0"
	"\x1c\x00\x00\x00\x63\xa5\x03\x4c"
	"random_kmalloc_seed\0"
	"\x18\x00\x00\x00\x19\x08\xda\x08"
	"kmalloc_caches\0\0"
	"\x18\x00\x00\x00\x4c\x48\xc3\xd0"
	"kmalloc_trace\0\0\0"
	"\x10\x00\x00\x00\x6d\x3e\x5a\xa8"
	"xa_load\0"
	"\x14\x00\x00\x00\x78\xe6\xbd\x2d"
	"proc_create\0"
	"\x18\x00\x00\x00\x19\x66\x52\xa5"
	"rb_insert_color\0"
	"\x10\x00\x00\x00\xa7\xb0\x39\x2d"
	"kstrdup\0"
	"\x14\x00\x00\x00\xcb\x69\x85\x8c"
	"kstrtoint\0\0\0"
	"\x10\x00\x00\x00\x6c\x9b\xdf\x85"
	"strsep\0\0"
	"\x14\x00\x00\x00\x23\xb3\xd9\xc1"
	"seq_read\0\0\0\0"
	"\x18\x00\x00\x00\x8c\x92\x66\x8e"
	"single_release\0\0"
	"\x18\x00\x00\x00\xbb\x90\x9b\x47"
	"param_ops_int\0\0\0"
	"\x18\x00\x00\x00\xd8\x63\x92\x84"
	"param_ops_charp\0"
	"\x14\x00\x00\x00\xbb\x6d\xfb\xbd"
	"__fentry__\0\0"
	"\x14\x00\x00\x00\x46\xbe\xb0\x46"
	"single_open\0"
	"\x1c\x00\x00\x00\xca\x39\x82\x5b"
	"__x86_return_thunk\0\0"
	"\x14\x00\x00\x00\x80\x2b\x0e\xc0"
	"seq_printf\0\0"
	"\x14\x00\x00\x00\xc2\x84\xe7\xec"
	"rb_first\0\0\0\0"
	"\x10\x00\x00\x00\xb5\x60\x93\xca"
	"rb_next\0"
	"\x18\x00\x00\x00\xeb\x7b\x33\xe1"
	"module_layout\0\0\0"
	"\x00\x00\x00\x00\x00\x00\x00\x00";

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "3B09C9834BE237ACB5730DA");
