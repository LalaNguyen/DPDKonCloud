
#include "hypervisor.h"


void proc_maps_push(proc_map_t ** head, ulong start, ulong end, int is_huge_page) {
	proc_map_t *new_node;
    new_node = malloc(sizeof(proc_map_t));
    new_node->start_addr = start;
    new_node->end_addr = end;
    new_node->is_huge_page = is_huge_page;
    new_node->next = *head;
    *head = new_node;
}

void push_prot_mem(prot_mem_t ** head_mem, ulong base, int size, int memtype) {
	prot_mem_t *new_node;
    new_node = malloc(sizeof(prot_mem_t));
    new_node->base_addr = base;
    new_node->size = size;
    new_node->memtype = memtype;    
    new_node->next = *head_mem;
    *head_mem = new_node;
}


proc_map_t * parse_proc_map(void){
	FILE* proc_maps = fopen("/proc/self/maps", "r");

	char line[256];
	ulong start_address;
	ulong end_address;
	int is_huge_page;

	proc_map_t *head = NULL;

	while (fgets(line, 256, proc_maps) != NULL)
	{
		if(strstr(line, "rtemap_") != NULL) {
			is_huge_page = 1;
		} else {
			is_huge_page = 0;
		}
		sscanf(line, "%08lx-%08lx\n", &start_address, &end_address);
		//printf("%08lx %08lx \n", start_address, end_address);
		proc_maps_push(&head, start_address, end_address, is_huge_page);
	}
	
	fclose(proc_maps);
	
	return head;
}

void do_remap(proc_map_t *head, int *lock, prot_mem_t *head_mem, prot_app_params_t * app){
		asm volatile ("vmcall\r\n"
				 : /* no output registers */
				 : "b" (QUIESCE_HYPERCALL), "a" (head), "c" (lock), "d"(head_mem), "S" (app)
				 : "memory" 
				 );
}

void proc_maps_print(proc_map_t *head) {
    proc_map_t *current = head;

    while (current != NULL) {
        printf("%08lx %08lx - %d \n", current->start_addr, current->end_addr, current->is_huge_page);
        current = current->next;
    }
}
