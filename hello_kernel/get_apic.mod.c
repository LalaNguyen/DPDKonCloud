#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xbc257400, "module_layout" },
	{ 0x47c7b0d2, "cpu_number" },
	{ 0xcc41bd7f, "apic" },
	{ 0x50eedeb8, "printk" },
	{ 0x115713a9, "pv_cpu_ops" },
	{ 0xd85df7d4, "boot_cpu_data" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "71ED6F67F490C0860B8FA9F");
