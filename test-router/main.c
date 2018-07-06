/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <string.h>
#include <sys/queue.h>
#include <stdarg.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>

#include <sys/mman.h>

#include <rte_common.h>
#include <rte_byteorder.h>
#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ring.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_lpm.h>
#include <rte_malloc.h>

#include "main.h"
#include "hypervisor.h"

int slave_resume_signal = 0;



int
main(int argc, char **argv)
{
	uint32_t lcore;
	int ret;
	proc_map_t * head;
	/* Init EAL */
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		return -1;
	argc -= ret;
	argv += ret;

	/* Parse application arguments (after the EAL ones) */
	ret = app_parse_args(argc, argv);
	if (ret < 0) {
		app_print_usage();
		return -1;
	}

	/* Init */
	app_init();

	/* Effectively disable on-demand paging and prevent swapping out page */
	mlockall(MCL_CURRENT | MCL_CURRENT);


	/* Launch per-lcore init on every lcore */
	rte_eal_mp_remote_launch(app_lcore_main_loop, NULL, SKIP_MASTER);

	RTE_LOG(INFO, USER1, "Core 0 waiting\n");

	#if DEBUG
	head = parse_proc_map();

	proc_maps_print(head);
		

	printf("Press enter to continue\n");
	
	char enter = 0;
	
	while(enter !='\r' && enter !='\n'){enter = getchar();}

	do_remap(head, &slave_resume_signal);
	#endif

	RTE_LCORE_FOREACH_SLAVE(lcore) {
		if (rte_eal_wait_lcore(lcore) < 0)
			return -1;
	}

	return 0;
}

int
app_lcore_main_loop(__attribute__((unused)) void *arg)
{
	unsigned lcore;
	lcore = rte_lcore_id();
	if (lcore == app.core_rx) {
		RTE_LOG(INFO, USER1, "Core %d is running RX\n", lcore);
		while(!slave_resume_signal);
		app_main_loop_rx();
	}
	if (lcore == app.core_worker) {
		RTE_LOG(INFO, USER1, "Core %d is running WORKER\n", lcore);
		switch (app.pipeline_type) {
			case e_APP_PIPELINE_ACL:
				app_main_loop_worker_pipeline_acl();
			return 0;
			case e_APP_PIPELINE_NONE:
			default:
				while(!slave_resume_signal);
				app_main_loop_worker();
				return 0;	
			}
		}

	if (lcore == app.core_tx) {
		RTE_LOG(INFO, USER1, "Core %d is running TX\n", lcore);
		while(!slave_resume_signal);
		app_main_loop_tx();
	}
	/* never return */
	return 0;
}
