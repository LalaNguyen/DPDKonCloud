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
	{ 0xe599fe4b, "device_remove_file" },
	{ 0x2320c022, "netdev_info" },
	{ 0xa98b8bdc, "kmalloc_caches" },
	{ 0x520cbdf, "pci_bus_read_config_byte" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0xf9a482f9, "msleep" },
	{ 0x68e941e1, "dcb_ieee_setapp" },
	{ 0x16117213, "pci_enable_sriov" },
	{ 0x528c709d, "simple_read_from_buffer" },
	{ 0x88d1b524, "debugfs_create_dir" },
	{ 0x2fe4e6d, "mem_map" },
	{ 0xd6ee688f, "vmalloc" },
	{ 0x76ebea8, "pv_lock_ops" },
	{ 0x3ec8886f, "param_ops_int" },
	{ 0x741e2a8f, "dcb_ieee_delapp" },
	{ 0xd0d8621b, "strlen" },
	{ 0xe2c8520, "skb_pad" },
	{ 0xbf373cf8, "page_address" },
	{ 0xbeb1ff81, "dev_set_drvdata" },
	{ 0x88a193, "__alloc_workqueue_key" },
	{ 0xe2e29ec3, "devm_kzalloc" },
	{ 0x2d37342e, "cpu_online_mask" },
	{ 0x5ff2f173, "dma_set_mask" },
	{ 0x89a226a9, "napi_complete" },
	{ 0x4acf2cbf, "pci_disable_device" },
	{ 0xc7a4fbed, "rtnl_lock" },
	{ 0x7f6522c0, "pci_disable_msix" },
	{ 0xc49090e3, "hwmon_device_unregister" },
	{ 0x1097d539, "netif_carrier_on" },
	{ 0xa4eb4eff, "_raw_spin_lock_bh" },
	{ 0x5839877, "pci_disable_sriov" },
	{ 0xb2102522, "skb_clone" },
	{ 0xc0a3d105, "find_next_bit" },
	{ 0xc406d922, "netif_carrier_off" },
	{ 0x4205ad24, "cancel_work_sync" },
	{ 0x33543801, "queue_work" },
	{ 0x85132430, "pci_dev_get" },
	{ 0xe094ef39, "sg_next" },
	{ 0x9bf83a3a, "x86_dma_fallback_dev" },
	{ 0x50c89f23, "__alloc_percpu" },
	{ 0x874d6130, "driver_for_each_device" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0xfb0e29f, "init_timer_key" },
	{ 0x999e8297, "vfree" },
	{ 0x5ece07e5, "pci_bus_write_config_word" },
	{ 0x4a50e597, "debugfs_create_file" },
	{ 0xb5aa7165, "dma_pool_destroy" },
	{ 0x47c7b0d2, "cpu_number" },
	{ 0x91715312, "sprintf" },
	{ 0x63434394, "debugfs_remove_recursive" },
	{ 0x9d1a89c7, "__alloc_pages_nodemask" },
	{ 0x9bdfc655, "pci_dev_driver" },
	{ 0x7cde1407, "netif_napi_del" },
	{ 0x77a60252, "sock_queue_err_skb" },
	{ 0x7d11c268, "jiffies" },
	{ 0xc9ec4e21, "free_percpu" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x733c3b54, "kasprintf" },
	{ 0xdcd98f59, "__netdev_alloc_skb" },
	{ 0x5995593, "__pskb_pull_tail" },
	{ 0xfe7c4287, "nr_cpu_ids" },
	{ 0x7dff84ea, "pci_set_master" },
	{ 0x2cded140, "dca3_get_tag" },
	{ 0xd5f2172f, "del_timer_sync" },
	{ 0x2bc95bd4, "memset" },
	{ 0xccc263a4, "dcb_getapp" },
	{ 0x9cf9d973, "dcb_setapp" },
	{ 0x39745baf, "pci_enable_pcie_error_reporting" },
	{ 0x2e471f01, "dca_register_notify" },
	{ 0xd3939204, "pci_enable_msix" },
	{ 0xe7af1c69, "pci_restore_state" },
	{ 0x8006c614, "dca_unregister_notify" },
	{ 0x723f9671, "dev_err" },
	{ 0x7c483ef2, "dev_addr_del" },
	{ 0x50eedeb8, "printk" },
	{ 0xe2793613, "ethtool_op_get_link" },
	{ 0x42224298, "sscanf" },
	{ 0x5152e605, "memcmp" },
	{ 0x8e0b7743, "ipv6_ext_hdr" },
	{ 0x93042e2f, "kunmap" },
	{ 0x7899b831, "free_netdev" },
	{ 0xb6ed1e53, "strncpy" },
	{ 0xf5031f5f, "register_netdev" },
	{ 0xb4390f9a, "mcount" },
	{ 0x6c2e3320, "strncmp" },
	{ 0x16305289, "warn_slowpath_null" },
	{ 0x8c03d20c, "destroy_workqueue" },
	{ 0x9e94987e, "dev_close" },
	{ 0x4c1182cb, "bitmap_scnprintf" },
	{ 0xa0af1e2f, "sk_free" },
	{ 0x621c95c1, "netif_set_real_num_rx_queues" },
	{ 0x8834396c, "mod_timer" },
	{ 0xc92bd263, "netif_set_real_num_tx_queues" },
	{ 0xdab1d7d, "netif_napi_add" },
	{ 0xf54c51a2, "dma_pool_free" },
	{ 0x652f2d76, "dcb_ieee_getapp_mask" },
	{ 0x199c3efa, "dma_release_from_coherent" },
	{ 0x2072ee9b, "request_threaded_irq" },
	{ 0xb2169f30, "dca_add_requester" },
	{ 0x28aff381, "__get_page_tail" },
	{ 0x6f86a208, "dev_kfree_skb_any" },
	{ 0x5f12014b, "contig_page_data" },
	{ 0xc5ffe7bb, "dma_alloc_from_coherent" },
	{ 0xe23a1319, "dev_open" },
	{ 0xe523ad75, "synchronize_irq" },
	{ 0xdd3323c4, "pci_find_capability" },
	{ 0xdc9170fc, "device_create_file" },
	{ 0x6c150662, "pci_select_bars" },
	{ 0x3ff62317, "local_bh_disable" },
	{ 0x46ccc456, "netif_device_attach" },
	{ 0x16996088, "napi_gro_receive" },
	{ 0x5e3f8835, "_dev_info" },
	{ 0x78adf159, "dev_addr_add" },
	{ 0x13857e11, "__free_pages" },
	{ 0xbb2d3a2a, "pci_disable_link_state" },
	{ 0xefbac1c9, "netif_device_detach" },
	{ 0x503be696, "__alloc_skb" },
	{ 0x3af98f9e, "ioremap_nocache" },
	{ 0x12a38747, "usleep_range" },
	{ 0xfcb77ba7, "pci_bus_read_config_word" },
	{ 0x22421a68, "kmap" },
	{ 0x92e8fd64, "__napi_schedule" },
	{ 0x473072d0, "pci_bus_read_config_dword" },
	{ 0x8bf826c, "_raw_spin_unlock_bh" },
	{ 0xbd5ed7db, "pci_cleanup_aer_uncorrect_error_status" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x579fbcd2, "cpu_possible_mask" },
	{ 0x676f2c86, "skb_checksum_help" },
	{ 0x28ef0e86, "kfree_skb" },
	{ 0x799aca4, "local_bh_enable" },
	{ 0xf7b56ba9, "eth_type_trans" },
	{ 0x8e888ec3, "cpumask_next_and" },
	{ 0xee3496c3, "dma_pool_alloc" },
	{ 0x28413e0e, "pskb_expand_head" },
	{ 0x73c65e81, "netdev_err" },
	{ 0x10d6ca4d, "netdev_features_change" },
	{ 0xb7015a1a, "pci_unregister_driver" },
	{ 0xcc5005fe, "msleep_interruptible" },
	{ 0xd6367922, "kmem_cache_alloc_trace" },
	{ 0x67f7403e, "_raw_spin_lock" },
	{ 0x7ecb001b, "__per_cpu_offset" },
	{ 0x7afa89fc, "vsnprintf" },
	{ 0xef642665, "pci_set_power_state" },
	{ 0xa15ec2e4, "netdev_warn" },
	{ 0x44366cfc, "simple_write_to_buffer" },
	{ 0xa77e4732, "eth_validate_addr" },
	{ 0x1fbf4f62, "pci_disable_pcie_error_reporting" },
	{ 0x37a0cba, "kfree" },
	{ 0x2e60bace, "memcpy" },
	{ 0x50f5e532, "call_rcu_sched" },
	{ 0xf59f197, "param_array_ops" },
	{ 0x8ed404d9, "pci_disable_msi" },
	{ 0xed42bba7, "dma_supported" },
	{ 0xb48e8de2, "pci_num_vf" },
	{ 0xedc03953, "iounmap" },
	{ 0x3894ac68, "pci_prepare_to_sleep" },
	{ 0x4833dfda, "__pci_register_driver" },
	{ 0x2288378f, "system_state" },
	{ 0x74c134b9, "__sw_hweight32" },
	{ 0xf7e13d5a, "pci_get_device" },
	{ 0xcf15f727, "dev_warn" },
	{ 0x475dc342, "unregister_netdev" },
	{ 0x9113ea8f, "pci_dev_put" },
	{ 0xb81960ca, "snprintf" },
	{ 0x86d39942, "pci_enable_msi_block" },
	{ 0xc4d04643, "__netif_schedule" },
	{ 0x7d50a24, "csum_partial" },
	{ 0x56f479f1, "consume_skb" },
	{ 0x49a3243a, "dca_remove_requester" },
	{ 0x21b57d7c, "pci_enable_device_mem" },
	{ 0x65a91c5f, "skb_tstamp_tx" },
	{ 0xb3acfdfc, "skb_put" },
	{ 0x5f06586f, "pci_wake_from_d3" },
	{ 0x2026f9b0, "pci_release_selected_regions" },
	{ 0x5e98feb7, "pci_request_selected_regions" },
	{ 0xa0208e02, "irq_set_affinity_hint" },
	{ 0xf76eba0, "__skb_tx_hash" },
	{ 0xe6cc1a76, "dma_pool_create" },
	{ 0xba815897, "skb_copy_bits" },
	{ 0xf8fe9c47, "dev_get_drvdata" },
	{ 0x590354f8, "hwmon_device_register" },
	{ 0xb97644b7, "pci_find_ext_capability" },
	{ 0x23fd3028, "vmalloc_node" },
	{ 0x6e720ff2, "rtnl_unlock" },
	{ 0x9e7d6bd0, "__udelay" },
	{ 0xc3c064d6, "dma_ops" },
	{ 0x8cad1691, "device_set_wakeup_enable" },
	{ 0xf20dabd8, "free_irq" },
	{ 0xded545b4, "pci_save_state" },
	{ 0xe914e41e, "strcpy" },
	{ 0xb7fbfe0f, "alloc_etherdev_mqs" },
	{ 0x52d24296, "netdev_crit" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=dca";

MODULE_ALIAS("pci:v00008086d000010B6sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000010C6sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000010C7sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000010C8sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d0000150Bsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000010DDsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000010ECsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000010F1sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000010E1sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000010F4sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000010DBsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d00001508sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000010F7sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000010FCsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d00001517sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000010FBsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d00001507sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d00001514sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000010F9sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d0000152Asv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d00001529sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d0000151Csv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000010F8sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d00001528sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d0000154Dsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d0000154Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d00001557sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d0000154Asv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d00001558sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d00001560sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d00001563sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000015D1sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000015AAsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000015B0sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000015ABsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000015ACsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000015ADsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000015AEsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000015C2sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000015C3sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000015C4sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000015C6sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000015C7sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000015C8sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000015CAsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000015CCsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000015CEsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000015E4sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000015E5sv*sd*bc*sc*i*");

MODULE_INFO(srcversion, "3FF473EED4A51455FFB196D");
