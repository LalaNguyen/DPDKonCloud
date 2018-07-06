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
#include <rte_lpm6.h>
#include <rte_malloc.h>
#include <rte_arp.h>
#include "main.h"

#define HYPERVISOR_PRINTF		0xc0215d03
int (* hyp_printf)(const char * fmt, ...) = (void *)HYPERVISOR_PRINTF;
// static void
// ipv4_addr_to_dot(uint32_t ipv4_addr)
// {
// 	printf("%d.%d.%d.%d\n", (ipv4_addr >> 24) & 0xFF,
// 		(ipv4_addr >> 16) & 0xFF, (ipv4_addr >> 8) & 0xFF,
// 		ipv4_addr & 0xFF);
// }

// static void print_pkt_info(struct rte_mbuf *m){
// 	struct ipv4_hdr *ip_hdr;
// 	uint32_t ip_dst, ip_src;
// 	uint8_t *m_data;
// 	struct ether_hdr * eth_hdr;
// 	struct ether_addr addr;
// 	printf(" m->port = %d\n", m->port);
// 	printf(" m->pkt_type = %d\n", m->packet_type);
// 	printf(" m->pkt_len = %d\n", m->pkt_len);

// 	eth_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);
// 	addr = eth_hdr->d_addr;
// 	printf("ether type = %d\n", eth_hdr->ether_type);
// 	printf("Port %u MAC: %02"PRIx8" %02"PRIx8" %02"PRIx8
// 			" %02"PRIx8" %02"PRIx8" %02"PRIx8"\n",
// 			(unsigned)m->port,
// 			addr.addr_bytes[0], addr.addr_bytes[1],
// 			addr.addr_bytes[2], addr.addr_bytes[3],
// 			addr.addr_bytes[4], addr.addr_bytes[5]);
// 	addr = eth_hdr->s_addr;
// 		printf("Port %u MAC: %02"PRIx8" %02"PRIx8" %02"PRIx8
// 			" %02"PRIx8" %02"PRIx8" %02"PRIx8"\n",
// 			(unsigned)m->port,
// 			addr.addr_bytes[0], addr.addr_bytes[1],
// 			addr.addr_bytes[2], addr.addr_bytes[3],
// 			addr.addr_bytes[4], addr.addr_bytes[5]);
// 	ip_hdr = (struct ipv4_hdr *)((uintptr_t)eth_hdr +  sizeof(struct ether_hdr));
// 	ip_dst = rte_be_to_cpu_32(ip_hdr->dst_addr);
// 	ip_src = rte_be_to_cpu_32(ip_hdr->src_addr);
// 	// ipv4_addr_to_dot(ip_dst);
// 	// ipv4_addr_to_dot(ip_src);
// }
static void
initialize_arp_header(struct arp_hdr *arp_hdr, struct ether_addr *src_mac,
		struct ether_addr *dst_mac, uint32_t src_ip, uint32_t dst_ip,
		uint32_t opcode)
{
	arp_hdr->arp_hrd = rte_cpu_to_be_16(ARP_HRD_ETHER);
	arp_hdr->arp_pro = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
	arp_hdr->arp_hln = ETHER_ADDR_LEN;
	arp_hdr->arp_pln = sizeof(uint32_t);
	arp_hdr->arp_op = rte_cpu_to_be_16(opcode);
	ether_addr_copy(src_mac, &arp_hdr->arp_data.arp_sha);
	arp_hdr->arp_data.arp_sip = src_ip;
	ether_addr_copy(dst_mac, &arp_hdr->arp_data.arp_tha);
	arp_hdr->arp_data.arp_tip = dst_ip;
}
static void
initialize_eth_header(struct ether_hdr *eth_hdr, struct ether_addr *src_mac,
		struct ether_addr *dst_mac, uint16_t ether_type,
		__rte_unused uint8_t vlan_enabled,__rte_unused uint16_t van_id)
{
	ether_addr_copy(dst_mac, &eth_hdr->d_addr);
	ether_addr_copy(src_mac, &eth_hdr->s_addr);
	eth_hdr->ether_type = rte_cpu_to_be_16(ether_type);
}

void
app_main_loop_rx(void) {
	uint32_t i;
	int ret;
	struct ether_hdr * eth_hdr;
	struct rte_mbuf *m; 
	uint16_t eth_type;
	uint32_t n_pkts;
	struct ipv4_hdr *ip_h;
	struct icmp_hdr *icmp_h;
	uint32_t ip_addr;
	struct arp_hdr *arp_m;


	#if DEBUG
	// hyp_printf("\n[CPU 0x02] Receiver enters hypervisor mode");
	#endif

	for (i = 0; ; i = ((i + 1) & (app.n_ports - 1))) {
		uint16_t n_mbufs;

		n_mbufs = rte_eth_rx_burst(
			app.ports[i],
			0,
			app.mbuf_rx.array,
			app.burst_size_rx_read);

		if (n_mbufs == 0)
			continue;

		/* There is a packet, check if it is ARP packet */
		m = app.mbuf_rx.array[0];
		eth_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);
		eth_type = RTE_BE_TO_CPU_16(eth_hdr->ether_type);
		arp_m = (struct arp_hdr *)((char *)eth_hdr + sizeof(struct ether_hdr));

		/* If the packet is ARP type */
		if( eth_type == ETHER_TYPE_ARP ){
			struct rte_mbuf *pkt;
			int pkt_size;
			struct arp_hdr *arp_pkt;
			struct ether_hdr * eth_pkt;
			uint32_t ip_src, ip_dst;
			/*
			 * Build ARP reply with new packet
			 */
        	pkt = rte_pktmbuf_alloc(app.pool);
        	pkt_size = sizeof(struct ether_hdr)+sizeof(struct arp_hdr);
        	pkt->data_len = pkt_size;
        	pkt->pkt_len = pkt_size;
        	pkt->port= i;
        	eth_pkt = rte_pktmbuf_mtod(pkt, struct ether_hdr *);
			arp_pkt = (struct arp_hdr *)((char *)eth_pkt + sizeof(struct ether_hdr));

        	initialize_eth_header(eth_pkt, &app.ip_ports[i]->mac_addr, &eth_hdr->s_addr, ETHER_TYPE_ARP, 0, 0);
			

			ip_src = app.ip_ports[i]->ip;
			ip_dst = arp_m->arp_data.arp_sip;

			/* Reply to ARP request from the port that it receives */
			initialize_arp_header(arp_pkt, &app.ip_ports[i]->mac_addr, &eth_hdr->s_addr, ip_src, ip_dst, ARP_OP_REPLY);
			n_pkts += rte_eth_tx_burst(i, 0, &pkt, 1);
			rte_pktmbuf_free(pkt);
			continue;
		}


		/* If the packet is IPv4 */
		ip_h = (struct ipv4_hdr *)((uintptr_t)eth_hdr +  sizeof(struct ether_hdr));
		icmp_h = (struct icmp_hdr *) ((char *)ip_h + sizeof(struct ipv4_hdr));


		/* If the packet is ICMP that target router */
		if ((ip_h->next_proto_id == IPPROTO_ICMP) &&
		       (icmp_h->icmp_type == IP_ICMP_ECHO_REQUEST)&&
		       (icmp_h->icmp_code == 0)&&
		       ((ip_h->dst_addr == app.ip_ports[i]->ip)||(ip_h->dst_addr == app.ip_ports[i^1]->ip))) {
			uint32_t cksum;
			struct ether_addr eth_addr;
			/*
			 * Prepare ICMP echo reply to be sent back.
		 	 * - switch ethernet source and destinations addresses,
		 	 * - use the request IP source address as the reply IP
		 	 *    destination address,
			 * - if the request IP destination address is a multicast
			 *   address:
			 *     - choose a reply IP source address different from the
			 *       request IP source address,
			 *     - re-compute the IP header checksum.
			 *   Otherwise:
			 *     - switch the request IP source and destination
			 *       addresses in the reply IP header,
			 *     - keep the IP header checksum unchanged.
			 * - set IP_ICMP_ECHO_REPLY in ICMP header.
			 * ICMP checksum is computed by assuming it is valid in the
			 * echo request and not verified.
			 */
			ether_addr_copy(&eth_hdr->s_addr, &eth_addr);
			ether_addr_copy(&eth_hdr->d_addr, &eth_hdr->s_addr);
			ether_addr_copy(&eth_addr, &eth_hdr->d_addr);
			ip_addr = ip_h->src_addr;
			ip_h->src_addr = ip_h->dst_addr;
			ip_h->dst_addr = ip_addr;
			icmp_h->icmp_type = IP_ICMP_ECHO_REPLY;
			cksum = ~icmp_h->icmp_cksum & 0xffff;
			cksum += ~htons(IP_ICMP_ECHO_REQUEST << 8) & 0xffff;
			cksum += htons(IP_ICMP_ECHO_REPLY << 8);
			cksum = (cksum & 0xffff) + (cksum >> 16);
			cksum = (cksum & 0xffff) + (cksum >> 16);
			icmp_h->icmp_cksum = ~cksum;
			n_pkts += rte_eth_tx_burst(i, 0, &m, 1);
			rte_pktmbuf_free(m);
			continue;
		}

		/* If the packet is normal IPv4 targeting different subnet */
		do {
				ret = rte_ring_sp_enqueue_bulk(
				app.rings_rx[i],
				(void **) app.mbuf_rx.array,
				n_mbufs);
		} while (ret < 0);
	}
}

void
app_main_loop_worker(void) {
	struct app_mbuf_array *worker_mbuf;
	uint32_t i;

	worker_mbuf = rte_malloc_socket(NULL, sizeof(struct app_mbuf_array),
			RTE_CACHE_LINE_SIZE, rte_socket_id());
	if (worker_mbuf == NULL)
		rte_panic("Worker thread: cannot allocate buffer space\n");

	#if DEBUG
	hyp_printf("\n[CPU 0x04] Worker enters hypervisor mode");
	#endif

	for (i = 0; ; i = ((i + 1) & (app.n_ports - 1))) {
		int ret;

		ret = rte_ring_sc_dequeue_bulk(
			app.rings_rx[i],
			(void **) worker_mbuf->array,
			app.burst_size_worker_read);

		if (ret == -ENOENT)
			continue;

		do {
			ret = rte_ring_sp_enqueue_bulk(
				app.rings_tx[i ^ 1],
				(void **) worker_mbuf->array,
				app.burst_size_worker_write);
		} while (ret < 0);
	}
}

void
app_main_loop_tx(void) {
	uint32_t i;
	#if DEBUG
	// hyp_printf("\n[CPU 0x06] TX enters hypervisor mode");
	#endif

	for (i = 0; ; i = ((i + 1) & (app.n_ports - 1))) {
		uint16_t n_mbufs, n_pkts;
		int ret;

		n_mbufs = app.mbuf_tx[i].n_mbufs;
		ret = rte_ring_sc_dequeue_bulk(
			app.rings_tx[i],
			(void **) &app.mbuf_tx[i].array[n_mbufs],
			app.burst_size_tx_read);

		if (ret == -ENOENT)
			continue;

		n_mbufs += app.burst_size_tx_read;

		if (n_mbufs < app.burst_size_tx_write) {
			app.mbuf_tx[i].n_mbufs = n_mbufs;
			continue;
		}

		n_pkts = rte_eth_tx_burst(
			app.ports[i],
			0,
			app.mbuf_tx[i].array,
			n_mbufs);

		if (n_pkts < n_mbufs) {
			uint16_t k;

			for (k = n_pkts; k < n_mbufs; k++) {
				struct rte_mbuf *pkt_to_free;

				pkt_to_free = app.mbuf_tx[i].array[k];
				rte_pktmbuf_free(pkt_to_free);
			}
		}

		app.mbuf_tx[i].n_mbufs = 0;
	}
}