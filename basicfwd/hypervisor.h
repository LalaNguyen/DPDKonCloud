#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define	QUIESCE_HYPERCALL		0x44550002

typedef struct proc_map {
	ulong start_addr;
	ulong end_addr;
	int is_huge_page;
	struct proc_map *next;
} proc_map_t;

typedef struct prot_mem {
	ulong base_addr;
	int size;
	int memtype;
	struct prot_mem *next;
} prot_mem_t;


typedef struct protect_params {
	/* CPU cores */
	uint32_t core_rx;
	uint32_t core_worker;
	uint32_t core_tx;

} prot_app_params_t;

// Function prototype
proc_map_t * parse_proc_map(void);
void do_remap(proc_map_t *, int *, prot_mem_t *, prot_app_params_t *);
void proc_maps_push(proc_map_t **, ulong, ulong, int);
void proc_maps_print(proc_map_t *);
void push_prot_mem(prot_mem_t ** , ulong , int , int );
