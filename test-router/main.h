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

#ifndef _MAIN_H_
#define _MAIN_H_

#define DEBUG 1

#ifndef APP_MBUF_ARRAY_SIZE
#define APP_MBUF_ARRAY_SIZE 256
#endif

struct app_mbuf_array {
	struct rte_mbuf *array[APP_MBUF_ARRAY_SIZE];
	uint16_t n_mbufs;
};

#ifndef APP_MAX_PORTS
#define APP_MAX_PORTS 4
#endif

struct ip_port_conf {
	uint8_t port_id;
	uint32_t ip;
	struct ether_addr mac_addr;
} __rte_cache_aligned;

struct app_params {
	/* CPU cores */
	uint32_t core_rx;
	uint32_t core_worker;
	uint32_t core_tx;

	/* Ports*/
	uint32_t ports[APP_MAX_PORTS];
	uint32_t n_ports;
	uint32_t port_rx_ring_size;
	uint32_t port_tx_ring_size;

	/* Rings */
	struct rte_ring *rings_rx[APP_MAX_PORTS];
	struct rte_ring *rings_tx[APP_MAX_PORTS];
	uint32_t ring_rx_size;
	uint32_t ring_tx_size;

	/* Internal buffers */
	struct app_mbuf_array mbuf_rx;
	struct app_mbuf_array mbuf_tx[APP_MAX_PORTS];

	/* Buffer pool */
	struct rte_mempool *pool;
	uint32_t pool_buffer_size;
	uint32_t pool_size;
	uint32_t pool_cache_size;

	/* Burst sizes */
	uint32_t burst_size_rx_read;
	uint32_t burst_size_rx_write;
	uint32_t burst_size_worker_read;
	uint32_t burst_size_worker_write;
	uint32_t burst_size_tx_read;
	uint32_t burst_size_tx_write;

	/* App behavior */
	uint32_t pipeline_type;

	/* rule path */
	char *rule_path;

	/* DPDK port with IP */
	struct ip_port_conf *ip_ports[2];

} __rte_cache_aligned;

extern struct app_params app;





/**
 * ICMP Header
 */
struct icmp_hdr {
	uint8_t  icmp_type;   /* ICMP packet type. */
	uint8_t  icmp_code;   /* ICMP packet code. */
	uint16_t icmp_cksum;  /* ICMP packet checksum. */
	uint16_t icmp_ident;  /* ICMP packet identifier. */
	uint16_t icmp_seq_nb; /* ICMP packet sequence number. */
} __attribute__((__packed__));

/* ICMP packet types */
#define IP_ICMP_ECHO_REPLY   0
#define IP_ICMP_ECHO_REQUEST 8


int app_parse_args(int argc, char **argv);
void app_print_usage(void);
void app_init(void);
int app_lcore_main_loop(void *arg);

/* Pipeline */
enum {
	e_APP_PIPELINE_NONE = 0,
	e_APP_PIPELINE_STUB,

	e_APP_PIPELINE_HASH_KEY8_EXT,
	e_APP_PIPELINE_HASH_KEY8_LRU,
	e_APP_PIPELINE_HASH_KEY16_EXT,
	e_APP_PIPELINE_HASH_KEY16_LRU,
	e_APP_PIPELINE_HASH_KEY32_EXT,
	e_APP_PIPELINE_HASH_KEY32_LRU,

	e_APP_PIPELINE_HASH_SPEC_KEY8_EXT,
	e_APP_PIPELINE_HASH_SPEC_KEY8_LRU,
	e_APP_PIPELINE_HASH_SPEC_KEY16_EXT,
	e_APP_PIPELINE_HASH_SPEC_KEY16_LRU,
	e_APP_PIPELINE_HASH_SPEC_KEY32_EXT,
	e_APP_PIPELINE_HASH_SPEC_KEY32_LRU,

	e_APP_PIPELINE_ACL,
	e_APP_PIPELINE_LPM,
	e_APP_PIPELINE_LPM_IPV6,
	e_APP_PIPELINES
};

void app_main_loop_rx(void);

void app_main_loop_worker(void);
void app_main_loop_worker_pipeline_acl(void);


void app_main_loop_tx(void);


#define APP_FLUSH 0
#ifndef APP_FLUSH
#define APP_FLUSH 0x3FF
#endif

#define APP_METADATA_OFFSET(offset) (sizeof(struct rte_mbuf) + (offset))
/*
 * Work-around of a compilation error with ICC on invocations of the
 * rte_be_to_cpu_16() function.
 */
#ifdef __GCC__
#define RTE_BE_TO_CPU_16(be_16_v)  rte_be_to_cpu_16((be_16_v))
#define RTE_CPU_TO_BE_16(cpu_16_v) rte_cpu_to_be_16((cpu_16_v))
#else
#if RTE_BYTE_ORDER == RTE_BIG_ENDIAN
#define RTE_BE_TO_CPU_16(be_16_v)  (be_16_v)
#define RTE_CPU_TO_BE_16(cpu_16_v) (cpu_16_v)
#else
#define RTE_BE_TO_CPU_16(be_16_v) \
	(uint16_t) ((((be_16_v) & 0xFF) << 8) | ((be_16_v) >> 8))
#define RTE_CPU_TO_BE_16(cpu_16_v) \
	(uint16_t) ((((cpu_16_v) & 0xFF) << 8) | ((cpu_16_v) >> 8))
#endif
#endif /* __GCC__ */

#endif /* _MAIN_H_ */
