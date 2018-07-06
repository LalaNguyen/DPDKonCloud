/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2016 Intel Corporation. All rights reserved.
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

#include <rte_log.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_byteorder.h>

#include <rte_port_ring.h>
#include <rte_table_acl.h>
#include <rte_pipeline.h>

#include "main.h"

#define DEFAULT_RULE_PATH			"rules.conf"
#define ACL_LEAD_CHAR				('@')
#define ROUTE_LEAD_CHAR				('R')
#define COMMENT_LEAD_CHAR			('#')
#define ROUTE_ENTRY_LINE_MEMBERS	7
#define ACL_ENTRY_LINE_MEMBERS		6
#define ROUTE_ENTRY_PRIORITY		0x1
#define ACL_ENTRY_PRIORITY			0x0
#define MaxIPv4String	16

#define GET_CB_FIELD(in, fd, base, lim, dlm)	do {            \
	unsigned long val;                                      \
	char *end;                                              \
	errno = 0;                                              \
	val = strtoul((in), &end, (base));                      \
	if (errno != 0 || end[0] != (dlm) || val > (lim))       \
		return -EINVAL;                               \
	(fd) = (typeof(fd))val;                                 \
	(in) = end + 1;                                         \
} while (0)

enum {
	PROTO_FIELD_IPV4,
	SRC_FIELD_IPV4,
	DST_FIELD_IPV4,
	SRCP_FIELD_IPV4,
	DSTP_FIELD_IPV4,
	NUM_FIELDS_IPV4
};

extern int slave_resume_signal;
/*
 * Meta-data of the ACL rules
 * 5 tuples: IP addresses, ports, and protocol.
 */
struct rte_acl_field_def ipv4_field_formats[NUM_FIELDS_IPV4] = {
	{
		.type = RTE_ACL_FIELD_TYPE_BITMASK,
		.size = sizeof(uint8_t),
		.field_index = PROTO_FIELD_IPV4,
		.input_index = PROTO_FIELD_IPV4,
		.offset = sizeof(struct ether_hdr) +
			offsetof(struct ipv4_hdr, next_proto_id),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = SRC_FIELD_IPV4,
		.input_index = SRC_FIELD_IPV4,
		.offset = sizeof(struct ether_hdr) +
			offsetof(struct ipv4_hdr, src_addr),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = DST_FIELD_IPV4,
		.input_index = DST_FIELD_IPV4,
		.offset = sizeof(struct ether_hdr) +
			offsetof(struct ipv4_hdr, dst_addr),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_RANGE,
		.size = sizeof(uint16_t),
		.field_index = SRCP_FIELD_IPV4,
		.input_index = SRCP_FIELD_IPV4,
		.offset = sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_RANGE,
		.size = sizeof(uint16_t),
		.field_index = DSTP_FIELD_IPV4,
		.input_index = SRCP_FIELD_IPV4,
		.offset = sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) +
			sizeof(uint16_t),
	},
};

static int parse_ipv4_net(const char *in, uint32_t *addr, uint32_t *mask_len) {
	uint8_t a, b, c, d, m;

	GET_CB_FIELD(in, a, 0, UINT8_MAX, '.');
	GET_CB_FIELD(in, b, 0, UINT8_MAX, '.');
	GET_CB_FIELD(in, c, 0, UINT8_MAX, '.');
	GET_CB_FIELD(in, d, 0, UINT8_MAX, '/');
	GET_CB_FIELD(in, m, 0, sizeof(uint32_t) * CHAR_BIT, 0);

	addr[0] = IPv4(a, b, c, d);
	mask_len[0] = m;

	return 0;
}

static int parse_range(const char *in, const char splitter, uint32_t *lo, uint32_t *hi) {
	uint32_t a, b;

	GET_CB_FIELD(in, a, 0, UINT32_MAX, splitter);
	GET_CB_FIELD(in, b, 0, UINT32_MAX, 0);

	lo[0] = a;
	hi[0] = b;

	return 0;
}

static inline int is_bypass_line(char *buff) {
	int i = 0;

	/* comment line */
	if (buff[0] == COMMENT_LEAD_CHAR)
		return 1;
	/* empty line */
	while (buff[i] != '\0') {
		if (!isspace(buff[i]))
			return 0;
		i++;
	}
	return 1;
}

static int parse_rule_members(char *str, char **in, int lim) {
	int i;
	char *s, *sp;
	static const char *dlm = " \t\n";
	s = str;

	for (i = 0; i != lim; i++, s = NULL) {
		in[i] = strtok_r(s, dlm, &sp);
		if (in[i] == NULL)
			return -EINVAL;
	}
	
	return 0;
}

static int add_route_rule(char *buff, struct rte_pipeline *p, uint32_t table_id) {
	char *in[ROUTE_ENTRY_LINE_MEMBERS];
	uint32_t addr, mask, lo, hi;
	int rc, key_found;

	parse_rule_members(buff, in, ROUTE_ENTRY_LINE_MEMBERS);

	struct rte_pipeline_table_entry table_entry = {
	 	.action = RTE_PIPELINE_ACTION_PORT,
	 	{.port_id = atoi(in[6])}
	};
	struct rte_table_acl_rule_add_params rule_params;
	struct rte_pipeline_table_entry *entry_ptr;

	memset(&rule_params, 0, sizeof(rule_params));

	printf("%s %s %s %s %s %s %s\n",
		in[0], in[1], in[2], in[3], in[4], in[5], in[6]);

	/* Set the rule values */
	parse_ipv4_net(in[1], &addr, &mask);
	rule_params.field_value[SRC_FIELD_IPV4].value.u32 = addr;
	rule_params.field_value[SRC_FIELD_IPV4].mask_range.u32 = mask;

	parse_ipv4_net(in[2], &addr, &mask);
	rule_params.field_value[DST_FIELD_IPV4].value.u32 = addr;
	rule_params.field_value[DST_FIELD_IPV4].mask_range.u32 = mask;

	parse_range(in[3], ':', &lo, &hi);
	rule_params.field_value[SRCP_FIELD_IPV4].value.u16 = lo;
	rule_params.field_value[SRCP_FIELD_IPV4].mask_range.u16 = hi;

	parse_range(in[4], ':', &lo, &hi);
	rule_params.field_value[DSTP_FIELD_IPV4].value.u16 = lo;
	rule_params.field_value[DSTP_FIELD_IPV4].mask_range.u16 = hi;

	parse_range(in[5], '/', &lo, &hi);
	rule_params.field_value[PROTO_FIELD_IPV4].value.u8 = lo;
	rule_params.field_value[PROTO_FIELD_IPV4].mask_range.u8 = hi;

	rule_params.priority = ROUTE_ENTRY_PRIORITY;

	rc = rte_pipeline_table_entry_add(p, table_id, &rule_params,
		&table_entry, &key_found, &entry_ptr);
	if (rc < 0)
		rte_panic("Unable to add entry to table %u (%d)\n",
				table_id, rc);

	return rc;
}

static void add_table_entries(struct rte_pipeline *p, uint32_t table_id) {
	char buff[LINE_MAX];

	FILE *fh = fopen(app.rule_path?app.rule_path:DEFAULT_RULE_PATH, "rb");
	unsigned int i = 0;

	if (fh == NULL)
		rte_exit(EXIT_FAILURE, "%s: Open %s failed\n", __func__,
			app.rule_path);
	
	i = 0;
	while (fgets(buff, LINE_MAX, fh) != NULL) {
		i++;

		if (is_bypass_line(buff))
			continue;

		char s = buff[0];

		/* Route entry */
		if (s == ROUTE_LEAD_CHAR) {
			add_route_rule(buff, p, table_id);
		}
		/* ACL entry */
		else if (s == ACL_LEAD_CHAR) {
			//add_acl_rule(buff, p, table_id);
		}
		/* Illegal line */
		else
			rte_exit(EXIT_FAILURE,
				"%s Line %u: should start with leading "
				"char %c or %c\n",
				app.rule_path, i, ROUTE_LEAD_CHAR, ACL_LEAD_CHAR);
	}

	fclose(fh);
}

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
// 	ipv4_addr_to_dot(ip_dst);
// 	ipv4_addr_to_dot(ip_src);
// }

// static void
// ether_addr_dump(const char *what, const struct ether_addr *ea)
// {
// 	char buf[ETHER_ADDR_FMT_SIZE];

// 	ether_format_addr(buf, ETHER_ADDR_FMT_SIZE, ea);
// 	if (what)
// 		printf("%s", what);
// 	printf("%s", buf);
// }

/* This function does not receive arp packet */
static inline int update_pkt_macaddr(__rte_unused struct rte_pipeline *p,
	struct rte_mbuf **pkts,
	__rte_unused uint64_t pkts_mask,
	__rte_unused struct rte_pipeline_table_entry **entries,
	__rte_unused void *arg){
	/* Read */
	struct rte_mbuf *m = pkts[0]; 
	// uint64_t dell_macaddr_dst = 0x479cc2c60e00; /* Hard coded dell address: 00:0e:c6:c2:9c:47 */
	// uint64_t acer_macaddr_dst = 0xdae5f5d53fc0; /* Hard coded Acer address: c0:3f:d5:f5:e5:da */
	uint64_t dell_macaddr_dst = 0xdab02101bfa4; //pc8
	uint64_t acer_macaddr_dst = 0x3bac2101bfa4; //pc9
	struct ether_hdr * eth_hdr;

	eth_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);
	
	if(RTE_ETH_IS_IPV4_HDR(m->packet_type)){
		struct ipv4_hdr *ip_hdr;
		uint32_t ip_dst;
		void *tmp;

		ip_hdr = (struct ipv4_hdr *)((uintptr_t)eth_hdr +  sizeof(struct ether_hdr));
		ip_dst = rte_be_to_cpu_32(ip_hdr->dst_addr);

		switch(ip_dst){
			case 0xc0a80321: /* If target IP is 192.168.3.33 */
				/* Overwrite the dst mac */
				tmp = &eth_hdr->d_addr.addr_bytes[0];
				*((uint64_t *)tmp) = acer_macaddr_dst;

				/* Overwrite the src mac to router MAC on the other port */
				ether_addr_copy(&app.ip_ports[(m->port)^1]->mac_addr, &eth_hdr->s_addr);
				break;

			case 0xc0a8042c: /* If target IP is 192.168.4.44 */
				/* Overwrite the dst mac */
				tmp = &eth_hdr->d_addr.addr_bytes[0];
				*((uint64_t *)tmp) = dell_macaddr_dst;

				/* Overwrite the src mac to router MAC on the other port */
				ether_addr_copy(&app.ip_ports[(m->port)^1]->mac_addr, &eth_hdr->s_addr);
				break;
		}
	}
	return 0;
}


static rte_pipeline_table_action_handler_hit
process_pkt(__rte_unused struct rte_pipeline *p)
{
	/* This handler is invoked once during table initialization */
	return update_pkt_macaddr;
}


void
app_main_loop_worker_pipeline_acl(void) {
	struct rte_pipeline_params pipeline_params = {
		.name = "pipeline",
		.socket_id = rte_socket_id(),
	};

	struct rte_pipeline *p;
	uint32_t port_in_id[APP_MAX_PORTS];
	uint32_t port_out_id[APP_MAX_PORTS];
	uint32_t table_id;
	uint32_t i;

	RTE_LOG(INFO, USER1,
		"Core %u is doing work (pipeline with ACL table)\n",
		rte_lcore_id());

	/* Pipeline configuration */
	p = rte_pipeline_create(&pipeline_params);
	if (p == NULL)
		rte_panic("Unable to configure the pipeline\n");

	/* Input port configuration */
	for (i = 0; i < app.n_ports; i++) {
		struct rte_port_ring_reader_params port_ring_params = {
			.ring = app.rings_rx[i],
		};

		struct rte_pipeline_port_in_params port_params = {
			.ops = &rte_port_ring_reader_ops,
			.arg_create = (void *) &port_ring_params,
			.f_action = NULL,
			.arg_ah = NULL,
			.burst_size = app.burst_size_worker_read,
		};

		if (rte_pipeline_port_in_create(p, &port_params,
			&port_in_id[i]))
			rte_panic("Unable to configure input port for "
				"ring %d\n", i);
	}

	/* Output port configuration */
	for (i = 0; i < app.n_ports; i++) {
		struct rte_port_ring_writer_params port_ring_params = {
			.ring = app.rings_tx[i],
			.tx_burst_sz = app.burst_size_worker_write,
		};

		struct rte_pipeline_port_out_params port_params = {
			.ops = &rte_port_ring_writer_ops,
			.arg_create = (void *) &port_ring_params,
			.f_action = NULL,
			.arg_ah = NULL,
		};

		if (rte_pipeline_port_out_create(p, &port_params,
			&port_out_id[i]))
			rte_panic("Unable to configure output port for "
				"ring %d\n", i);
	}

	/* Table configuration */
	{
		struct rte_table_acl_params table_acl_params = {
			.name = "test", /* unique identifier for acl contexts */
			.n_rules = 1 << 5,
			.n_rule_fields = DIM(ipv4_field_formats),
		};

		/* Copy in the rule meta-data defined above into the params */
		memcpy(table_acl_params.field_format, ipv4_field_formats,
			sizeof(ipv4_field_formats));

		struct rte_pipeline_table_params table_params = {
			.ops = &rte_table_acl_ops,
			.arg_create = &table_acl_params,
			.f_action_hit = process_pkt(p),
			.f_action_miss = NULL,
			.arg_ah = NULL,
			.action_data_size = 0,
		};

		if (rte_pipeline_table_create(p, &table_params, &table_id))
			rte_panic("Unable to configure the ACL table\n");
	}

	/* Interconnecting ports and tables */
	for (i = 0; i < app.n_ports; i++)
		if (rte_pipeline_port_in_connect_to_table(p, port_in_id[i],
			table_id))
			rte_panic("Unable to connect input port %u to "
				"table %u\n", port_in_id[i],  table_id);
	
	/* Add entries to tables */
	add_table_entries(p, table_id);
	
	/* Enable input ports */
	for (i = 0; i < app.n_ports; i++)
		if (rte_pipeline_port_in_enable(p, port_in_id[i]))
			rte_panic("Unable to enable input port %u\n",
				port_in_id[i]);

	/* Check pipeline consistency */
	if (rte_pipeline_check(p) < 0)
		rte_panic("Pipeline consistency check failed\n");
			
	while(!slave_resume_signal);
	/* Run-time */
#if APP_FLUSH == 0
	for ( ; ; )
		rte_pipeline_run(p);
#else
	for (i = 0; ; i++) {
		rte_pipeline_run(p);

		if ((i & APP_FLUSH) == 0)
			rte_pipeline_flush(p);
	}
#endif
}
