cmd_basicfwd = gcc -o basicfwd -m32 -pthread  -march=native -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 -DRTE_MACHINE_CPUFLAG_AES -DRTE_MACHINE_CPUFLAG_PCLMULQDQ -DRTE_MACHINE_CPUFLAG_AVX  -I/home/minh/DPDKonCloud/skeleton/build/include -I/home/minh/dpdk16.07.2/i686-native-linuxapp-gcc/include -include /home/minh/dpdk16.07.2/i686-native-linuxapp-gcc/include/rte_config.h -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wpointer-arith -Wcast-align -Wnested-externs -Wcast-qual -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings -Wno-missing-field-initializers -Wno-uninitialized basicfwd.o hypervisor.o -L/home/minh/dpdk16.07.2/i686-native-linuxapp-gcc/lib -Wl,-lrte_pipeline -Wl,-lrte_table -Wl,-lrte_port -Wl,-lrte_distributor -Wl,-lrte_reorder -Wl,-lrte_ip_frag -Wl,-lrte_meter -Wl,-lrte_sched -Wl,-lrte_lpm -Wl,--whole-archive -Wl,-lrte_acl -Wl,--no-whole-archive -Wl,-lrte_jobstats -Wl,--whole-archive -Wl,-lrte_timer -Wl,-lrte_hash -Wl,-lrte_kvargs -Wl,-lrte_mbuf -Wl,-lethdev -Wl,-lrte_mempool -Wl,-lrte_ring -Wl,-lrte_eal -Wl,-lrte_cmdline -Wl,-lrte_cfgfile -Wl,-lrte_pmd_af_packet -Wl,-lrte_pmd_ixgbe -Wl,-lrte_pmd_null -Wl,-lrte_pmd_ring -Wl,--no-whole-archive -Wl,-lrt -Wl,-lm -Wl,-ldl -Wl,-melf_i386 -Wl,-export-dynamic -Wl,-export-dynamic -L/home/minh/DPDKonCloud/skeleton/build/lib -L/home/minh/dpdk16.07.2/i686-native-linuxapp-gcc/lib -Wl,--as-needed -Wl,-Map=basicfwd.map -Wl,--cref 
