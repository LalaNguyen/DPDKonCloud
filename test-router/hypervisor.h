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

// Function prototype
proc_map_t * parse_proc_map(void);
void do_remap(proc_map_t *, int *);
void proc_maps_push(proc_map_t **, ulong, ulong, int);
void proc_maps_print(proc_map_t *);
