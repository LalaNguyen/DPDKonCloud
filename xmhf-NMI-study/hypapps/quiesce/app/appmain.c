/*
 * @XMHF_LICENSE_HEADER_START@
 *
 * eXtensible, Modular Hypervisor Framework (XMHF)
 * Copyright (c) 2009-2012 Carnegie Mellon University
 * Copyright (c) 2010-2012 VDG Inc.
 * All Rights Reserved.
 *
 * Developed by: XMHF Team
 *               Carnegie Mellon University / CyLab
 *               VDG Inc.
 *               http://xmhf.org
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the names of Carnegie Mellon or VDG Inc, nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @XMHF_LICENSE_HEADER_END@
 */

// appmain.c
// xmhf application main module
// author: amit vasudevan (amitvasudevan@acm.org)

#include "appmain.h"

void print_list(struct proc_map_page_4K*);

typedef struct {
	void *buf;
	void *page_base;
	int num_allocd;
	int num_used;
} page_pool_t;

page_pool_t * ppool = NULL;
//page_pool_t * hpool = NULL;

static inline void store_idt(struct desc_ptr *dtr){
 asm volatile("sidt %0":"=m" (*dtr));
}

void dump_xmhf_idt(u32 idt_base, u32 size){
 u64 *idt;
 u32 i;
 (void)size;
 /* Dump current GDT information */
 printf("\n------Dumping IDT-----");
 idt = (u64*)idt_base;
 for (i = 0; i <= 128; i++) {
   if(*(idt+i)!=0 || i==0)
     printf("\n<0x%lx> IDT[%d]= 0x%llx", idt+i, i, *(idt+i));
 }
}


void add_systemcall(){
  u32 * pexceptionstubs;
  idtentry_t *idtentry;
  struct desc_ptr idt_base; 

  pexceptionstubs=(u32 *)&xmhf_xcphandler_exceptionstubs;

  idtentry=(idtentry_t *)((u32)xmhf_xcphandler_arch_get_idt_start()+ (128*8));
  idtentry->isrLow= (u16)pexceptionstubs[32];
  idtentry->isrHigh= (u16) ( (u32)pexceptionstubs[32] >> 16 );
  idtentry->isrSelector = __CS;
  idtentry->count=0x0;
  idtentry->type=0xEE;  //32-bit interrupt gate
                //present=1, DPL=00b, system=0, type=1110b 
  printf("\n<0x%lx> idt[%d] = 0x%llx", (u32)xmhf_xcphandler_arch_get_idt_start()+ (128*8), 128, *idtentry);

  idtentry=(idtentry_t *)((u32)xmhf_xcphandler_arch_get_idt_start()+ (239*8));
  idtentry->isrLow= (u16)pexceptionstubs[33];
  idtentry->isrHigh= (u16) ( (u32)pexceptionstubs[33] >> 16 );
  idtentry->isrSelector = __CS;
  idtentry->count=0x0;
  idtentry->type=0xEE;  //32-bit interrupt gate
                //present=1, DPL=00b, system=0, type=1110b 
  printf("\n<0x%lx> idt[%d] = 0x%llx", (u32)xmhf_xcphandler_arch_get_idt_start()+ (239*8), 239, *idtentry);

  store_idt(&idt_base); // get gdt base and size
  idt_base.size = 2047;
  asm volatile ("lidt %0\r\n"::"m"(idt_base));
  printf("\nnew idt size %d", idt_base.size+1);
  printf("\nnew idt address 0x%lx", idt_base.address);
  //dump_xmhf_idt(idt_base.address, idt_base.size+1);
}

int update_hypervisor_pt(VCPU *vcpu, struct proc_map_page_4K *start){    proc_map_page_4K *head_hva = start;
    //PAE paging used by host
    u32 hcr3;
    u32 pdpt_index, pd_index, pt_index;
    u64 paddr;
    pdpt_t hpdpt;
    pdt_t hpd; 
    pt_t hpt, new_pt; 
    u32 pdpt_entry, pd_entry;
    u32 tmp, vaddr;
    (void) vcpu;
    hcr3 = (u32)read_cr3();
    // get fields from virtual addr 
    tmp = pae_get_addr_from_32bit_cr3(hcr3);
    hpdpt = (pdpt_t)spa2hva1((u32)tmp); 
    pagelist_init(app_pl);
    //printf("\n%s:total mem mallocd: %u",__FUNCTION__, heapmem_get_used_size());
    while(head_hva->next!=NULL || head_hva->next!=0){
        vaddr = head_hva->vaddr;
        paddr = head_hva->paddr;
	 //printf("\n%s: vaddr: 0x%lx --> paddr 0x%lx",__FUNCTION__, vaddr, paddr);
          pdpt_index = pae_get_pdpt_index(vaddr);
          pd_index = pae_get_pdt_index(vaddr);
          pt_index = pae_get_pt_index(vaddr);

          //grab pdpt entry
          pdpt_entry = hpdpt[pdpt_index];
          // printf("\n%s: <%p>: hpdpt[%d]=%p",__FUNCTION__, &hpdpt[pdpt_index], pdpt_index, pdpt_entry);
          //grab pd entry
          tmp = pae_get_addr_from_pdpe(pdpt_entry);
          hpd = (pdt_t)spa2hva1((u32)tmp); 
          pd_entry = hpd[pd_index];
          // printf("\n%s: <%p>: hpd[%d]=%p",__FUNCTION__, &hpd[pd_index], pd_index, pd_entry);
          // Originally, page table uses PAE, check if its PAE,k then replace with 32-bit paging
          if (head_hva->is_huge_page == 0){
             if(pd_entry & _PAGE_PSE){
                //Allocate new page;
                new_pt = pagelist_get_zeroedpage(app_pl);
                new_pt[pt_index] = paddr | (u64)(_PAGE_PRESENT | _PAGE_RW | _PAGE_USER);
                hpd[pd_index] = (u64)hva2spa1(new_pt) | (u64)(_PAGE_PRESENT | _PAGE_RW | _PAGE_USER);
              }
             else{
                //Page is created, simply graph the entry
                tmp = (u32)npae_get_addr_from_pde(pd_entry);
                hpt = (pt_t)spa2hva1((u32)tmp);  
                hpt[pt_index] =  paddr | (u64)(_PAGE_PRESENT | _PAGE_RW | _PAGE_USER);
                //printf("\n%s: Found pd_entry at = 0x%lx",__FUNCTION__, pd_entry);
              }
          }
          else if (head_hva->is_huge_page == 1){
              if(pd_entry & _PAGE_PSE)
                hpd[pd_index] = paddr | (u64)(_PAGE_PRESENT | _PAGE_RW | _PAGE_PSE | _PAGE_USER);
              else
                printf("This is mapped by a 4KB table\n");
          }
         

      head_hva = head_hva->next;
    }
  //print_host_table();
  return 0;
}

void page_pool_init(page_pool_t *pp){
    const int pages = 10000;
    pp->buf=(void*)reserved_page_pool;
    pp->page_base=(void*)reserved_page_pool;
    pp->num_allocd=pages;
    pp->num_used=0;	
    printf("\nPage pool initialized at address 0x%p!",pp->page_base);
}

void huge_page_pool_init(page_pool_t *hp){
   const int pages = 100;
   hp->buf=(void*)reserved_huge_page_pool;
   hp->page_base=(void*)reserved_huge_page_pool;
   hp->num_allocd= pages;
   hp->num_used=0;
   printf("\nHuge pool initialized at address %p!",hp->page_base);
}
void *get_page_from_page_pool(page_pool_t *pp){
    void *page; 
    page = pp->page_base+(pp->num_used*PAGE_SIZE_4K);
    pp->num_used++;
    return page;
}

void *get_huge_page_from_page_pool(page_pool_t *hp){
    void *huge_page;
    huge_page = hp->page_base+(hp->num_used*PAGE_SIZE_2M);
    hp->num_used++;
    return huge_page;
}

void *get_zeroed_page(page_pool_t *pp){
	void *page = get_page_from_page_pool(pp);
	memset(page,0,PAGE_SIZE_4K);
	return page;
}

void *get_zeroed_huge_page(page_pool_t *hp){
	void *huge_page = get_huge_page_from_page_pool(hp);
	memset(huge_page,0, PAGE_SIZE_2M);
	return huge_page;
}

void *clone_page(void * src_page, page_pool_t *pp){
	void *dst_page = get_zeroed_page(pp);
	memcpy(dst_page,src_page,PAGE_SIZE_4K);
	return dst_page;
}

void *clone_huge_page(void *src_page, page_pool_t *hp){
	void *dst_page = get_zeroed_huge_page(hp);
	memcpy(dst_page, src_page,PAGE_SIZE_2M);
	return dst_page;
}

proc_map_page_4K * get_proc_map_from_guest_app(VCPU *vcpu, struct regs *r){
  proc_map_t *head_gva;
  proc_map_page_4K *head_hva;
  int npages, i = 0;
  u32 host_vaddr, host_paddr;
  /* printing purposes */
  //u32 tmp1, tmp3;
  u32 tmp2;
  //void *src_page;
  void  *cp_page;
  printf("\nCopy the list header from guest to host");
  mem_init();
  page_pool_init(ppool);
  //huge_page_pool_init(hpool);
  head_gva = malloc(sizeof(proc_map_t));
  head_hva = malloc(sizeof(proc_map_page_4K));
  /* marking the last entry with 0 */
  memset(head_hva, 0 , sizeof(proc_map_page_4K));
  
  copy_from_gva_to_hva(vcpu, r->eax, head_gva, sizeof(proc_map_t));
  
  while(head_gva->next!=NULL){
      // printf("\n 0x%lx-0x%lx , is_huge_page = %d", head_gva->start_addr,head_gva->end_addr, head_gva->is_huge_page);
      if (head_gva->is_huge_page == 0){
          npages = (head_gva->end_addr-head_gva->start_addr)/PAGE_SIZE_4K;
	  //printf("\n 0x%lx - 0x%lx, number of pages = %d", host_vaddr, host_paddr, npages);
          host_vaddr = head_gva->start_addr;
          for (i = 0; i < npages; i++){
	    if(host_vaddr < 0xB0000000 || host_vaddr >0xB6F00000){
	    host_paddr = (u32)gpa2hva((u32)gva2gpa(vcpu, (u32)host_vaddr));
            //src_page = spa2hva((spa_t)host_paddr);
            //tmp1 = (u32)src_page;
	    cp_page = clone_page((void*)host_paddr,ppool);
            //tmp3 = (u32)cp_page;
	    tmp2 =  hva2spa1(cp_page);

	    /*if(host_vaddr <= 0x400000){
	    printf("\nBefore clone: 0x%lx --> 0x%lx <---0x%lx", tmp1, host_paddr,host_vaddr);
            printf("\n");
            //print_bytes((gva_t*)src_page,18);
	    printf("\nAfter clone: 0x%lx --> 0x%lx", tmp3, tmp2);
	    printf("\n");
	    //print_bytes((gva_t*)cp_page,18);
	    }*/
            //push_pa_map(&head_hva, host_vaddr, host_paddr,head_gva->is_huge_page);
	    push_pa_map(&head_hva, host_vaddr,tmp2, head_gva->is_huge_page);
            host_vaddr += PAGE_SIZE_4K;
	    }
	    else{
			//printf("\nUpdate Mapping back to Guest");
		        host_paddr = (u32)gpa2hva((u32)gva2gpa(vcpu, (u32)host_vaddr));
			push_pa_map(&head_hva, host_vaddr, host_paddr,head_gva->is_huge_page);
            		host_vaddr += PAGE_SIZE_4K;
	    }
        }
      }
      else if (head_gva->is_huge_page == 1){
          npages = (head_gva->end_addr-head_gva->start_addr)/PAGE_SIZE_2M;
          host_vaddr = head_gva->start_addr;
          for (i = 0; i < npages; i++){
            host_paddr = (u32)gpa2hva((u32)gva2gpa(vcpu, (u32)host_vaddr));
	    /* clone huge page*/
	    //cp_page = clone_huge_page((void*)host_paddr,ppool);
	    //tmp2 = hva2spa1(cp_page);
            //printf("\n Clone %p to %p", host_paddr, tmp2);
	    //push_pa_map(&head_hva, host_vaddr, tmp2, head_gva->is_huge_page);
            push_pa_map(&head_hva, host_vaddr, host_paddr, head_gva->is_huge_page);
            host_vaddr += PAGE_SIZE_2M;
        }
      }

      // copy new proc_map_t to the head_gva
      copy_from_gva_to_hva(vcpu, (u32)head_gva->next, head_gva, sizeof(proc_map_t));
  }
  printf("\nNumber of page withdrawed from the pool: %d\n", ppool->num_used);
  // Push the last item
  npages = (head_gva->end_addr-head_gva->start_addr)/PAGE_SIZE_4K;
  //printf("\nLast page is %p - %p, %d", head_gva->start_addr, head_gva->end_addr, head_gva->is_huge_page);
  host_vaddr = head_gva->start_addr;

  for (i = 0; i < npages; i++){
          host_paddr = (u32)gpa2hva((u32)gva2gpa(vcpu, (u32)host_vaddr));
	  cp_page = clone_page((void*)host_paddr,ppool);
          tmp2 =  hva2spa1(cp_page);
	  push_pa_map(&head_hva, host_vaddr,tmp2, head_gva->is_huge_page);
          //push_pa_map(&head_hva, host_vaddr, host_paddr, head_gva->is_huge_page);
          host_vaddr += PAGE_SIZE_4K;
      }
  //print_list(head_hva);
  return head_hva;
}

// application main
u32 xmhf_app_main(VCPU *vcpu, APP_PARAM_BLOCK *apb){
  (void)apb;	//unused
  printf("\nCPU(0x%02x): Hello world from Quiesce XMHF hyperapp!", vcpu->id);
  return APP_INIT_SUCCESS;  //successful
}

void prepare_user_space_context(VCPU *vcpu){
  printf("\nCPU(0x%02x): Preparing CPU context!", vcpu->id);
  // xmhf_reload_gdt(vcpu);
  add_systemcall();
}

//returns APP_SUCCESS if we handled the hypercall else APP_ERROR
u32 xmhf_app_handlehypercall(VCPU *vcpu, struct regs *r){
	u32 status=APP_SUCCESS;
	u32 hypercall_number = r->ebx;
	//u32 * p1 = (u32*)0x806455b;
	//u32 * p2 = (u32*)0x806400d;
  // Get address of slave_resume_signal
  u32 *slave_resume_signal_p = (u32*) r->ecx;
  proc_map_page_4K * head_list;
	switch(hypercall_number){
		case QUIESCE_HYPERCALL:
      // Get proc mapping from guest application
      // print_guest_table((gva_t)vcpu->vmcs.guest_CR3);
      head_list = get_proc_map_from_guest_app(vcpu, r);
      // Update the hypervisor page table on this running core (i.e. lcore 0)
      update_hypervisor_pt(vcpu, head_list); 
     //{ u32 *p3 = (u32*) 0x80bb6e3;
     //  print_bytes(p3,20);
     //}
	// print_bytes(p1,20);
     // printf("\n");
     // print_bytes(p2,20);
      // Set resume signal on slave to 1
      *slave_resume_signal_p = 1;
      // Prepare user space context
      prepare_user_space_context(vcpu);
			break;
			
		default:
			printf("\nCPU(0x%02x): unhandled hypercall (0x%08x)!", vcpu->id, r->ebx);
			break;
	}
	return status;
}

//handles XMHF shutdown callback
//note: should not return
void xmhf_app_handleshutdown(VCPU *vcpu, struct regs *r){
	(void)r; //unused
	xmhf_baseplatform_reboot(vcpu);				
}

//handles h/w pagetable violations
//for now this always returns APP_SUCCESS
u32 xmhf_app_handleintercept_hwpgtblviolation(VCPU *vcpu,
      struct regs *r,
      u64 gpa, u64 gva, u64 violationcode){
	u32 status = APP_SUCCESS;

	(void)vcpu; //unused
	(void)r; //unused
	(void)gpa; //unused
	(void)gva; //unused
	(void)violationcode; //unused

	return status;
}


//handles i/o port intercepts
//returns either APP_IOINTERCEPT_SKIP or APP_IOINTERCEPT_CHAIN
u32 xmhf_app_handleintercept_portaccess(VCPU *vcpu, struct regs *r, 
  u32 portnum, u32 access_type, u32 access_size){
	(void)vcpu; //unused
	(void)r; //unused
	(void)portnum; //unused
	(void)access_type; //unused
	(void)access_size; //unused

 	return APP_IOINTERCEPT_CHAIN;
}


/********************************/
/* Address Conversion Functions */
/********************************/

u64 gva2gpa(VCPU * vcpu, gva_t vaddr)
{
    u32 kcr3 = (u32)vcpu->vmcs.guest_CR3;
    u32 pd_index, pt_index, offset;
    u64 paddr;
    npdt_t kpd; 
    npt_t kpt; 
    u32 pd_entry, pt_entry;
    u32 tmp;
 
    /* Guest uses 32-bit paging */
    // get fields from virtual addr 
    pd_index = npae_get_pdt_index(vaddr);
    // printf("\npd_index = 0x%lx -> integer:%d",pd_index, pd_index);
    pt_index = npae_get_pt_index(vaddr);
    // printf("\npt_index = 0x%lx",pt_index);
    // printf("\noffset = 0x%lx",offset);
    // grab pd entry
    tmp = npae_get_addr_from_32bit_cr3(kcr3);
    // printf("\ntmp = 0x%lx, kcr3 = 0x%lx",tmp, kcr3);
    kpd = (npdt_t)((u32)tmp); 
    // printf("\nkpd = <phys> 0x%lx, <hva> 0x%lx", tmp, kpd);
    pd_entry = kpd[pd_index];
 
    if (pd_entry & _PAGE_PSE){
        offset = npae_get_offset_big(vaddr);  
        paddr = (u64)npae_get_addr_from_pte(pd_entry) + offset;
          // printf("\n The gva is in kpd[%d] = 0x%lx",pd_index, pd_entry);
    }
    else {
        offset = npae_get_offset_4K_page(vaddr);  
        tmp = (u32)npae_get_addr_from_pde(pd_entry);
        kpt = (npt_t)((u32)tmp);  
        pt_entry  = kpt[pt_index];
          // printf("\n pt[%d] = 0x%lx",pt_index, pt_entry);
          // find physical page base addr from page table entry 
        paddr = (u64)npae_get_addr_from_pte(pt_entry) + offset;
          // printf("\n pa = 0x%lx",paddr);
    }
  // printf("\nTranslating gva=0x%lx to gpa=0x%lx",vaddr, paddr);
  return paddr;
}

u64 get_hpa_from_hva(u32 vaddr){
    u32 hcr3;
    u32 pdpt_index, pd_index, pt_index;
    pdpt_t hpdpt;
    pdt_t hpd; 
    pt_t hpt; 
    u32 pdpt_entry, pd_entry, pt_entry;
    u32 tmp;
    u32 offset;

    hcr3 = (u32)read_cr3();
    // get fields from virtual addr 
    tmp = pae_get_addr_from_32bit_cr3(hcr3);
    hpdpt = (pdpt_t)spa2hva1((u32)tmp); 
    pdpt_index = pae_get_pdpt_index(vaddr);
    pd_index = pae_get_pdt_index(vaddr);
    pt_index = pae_get_pt_index(vaddr);
    offset = pae_get_offset_4K_page(vaddr);
          //grab pdpt entry
          pdpt_entry = hpdpt[pdpt_index];
          // printf("\n%s: <%p>: hpdpt[%d]=%p",__FUNCTION__, &hpdpt[pdpt_index], pdpt_index, pdpt_entry);
          //grab pd entry
          tmp = pae_get_addr_from_pdpe(pdpt_entry);
          hpd = (pdt_t)spa2hva1((u32)tmp); 
          pd_entry = hpd[pd_index];
          // printf("\n%s: <%p>: hpd[%d]=%p",__FUNCTION__, &hpd[pd_index], pd_index, pd_entry);
          // Originally, page table uses PAE, check if its PAE,k then replace with 32-bit paging
     
          //Page is created, simply graph the entry
          tmp = (u32)npae_get_addr_from_pde(pd_entry);
          hpt = (pt_t)spa2hva1((u32)tmp);

          pt_entry = hpt[pt_index] & 0xFFFFF000;
          return (pt_entry+offset);
          
}

/*********************/
/* Utility Functions */
/*********************/
/* function push a new node to linked list */
void push_pa_map(proc_map_page_4K ** head, u32 vaddr, u32 paddr, u32 is_huge_page) {
    proc_map_page_4K *new_node;
    new_node = malloc(sizeof(proc_map_page_4K));
    new_node->vaddr = vaddr;
    new_node->paddr = paddr;
    new_node->is_huge_page = is_huge_page;
    new_node->next = *head;
    *head = new_node;
}

/* function to swap data of two nodes a and b*/
void swap(struct proc_map_page_4K *a, struct proc_map_page_4K *b)
{
    int temp1 = a->vaddr;
    int temp2 = a->paddr;
    a->vaddr = b->vaddr;
    a->paddr = b->paddr;
    b->vaddr = temp1;
    b->paddr = temp2;
}


/* Bubble sort the given linked list */
void bubbleSort(struct proc_map_page_4K *start)
{
    int swapped;
    proc_map_page_4K *ptr1;
    proc_map_page_4K *lptr = NULL;
    /* Checking for empty list */
    do
    {
        swapped = 0;
        ptr1 = start;
        while (ptr1->next != lptr)
        {
            if (ptr1->vaddr > ptr1->next->vaddr && ptr1->next->next != 0)
            {
                swap(ptr1, ptr1->next);
                swapped = 1;
            }
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    }
    while (swapped);
}

void copy_from_gva_to_hva(VCPU *vcpu, gva_t vaddr, void *dst, u32 len){
    void *src_hva = gpa2hva(gva2gpa(vcpu,vaddr));
    memcpy(dst, src_hva, len);
}



/*******************/
/* Debug Functions */
/*******************/

void print_guest_table(gva_t cr3)
{
    int i = 0;
    int j = 0;
    u32 *pt;
    u32* pd = (u32*) cr3;
    printf("\nInspect kernel CR3: 0x%lx", cr3);
    for (; i < 1024; i++)
    {
        if (pd[i] != 0)
        {
            printf("\nPDE %X: %X\n", i, pd[i]);
            pt = (u32*) (pd[i] & ~(0xFFF));
            j = 0;
            for (; j < 50; j++)
            {
                if (pt[j] != 0)
                    printf("\tPTE %X: %X\n", j, pt[j]);
            }
        }
    }
}


void print_bytes (gva_t* start, int len)
{
    unsigned int i = 0;
    for (; i < len / sizeof (gva_t); i ++)
    {
        printf ("%08lX ", start[i]);
        if ((i % 8) == 7) printf ("\n");
    }
}

void print_host_table(void){
    u32 hcr3;
    pdpt_t hpdpt;
    pdt_t hpd; 
    pt_t hpt; 
    u32 tmp;
    u32 i, j, z;
    hcr3 = (u32)read_cr3();
    tmp = pae_get_addr_from_32bit_cr3(hcr3);
    hpdpt = (pdpt_t)spa2hva1((u32)tmp); 
    for (i = 0; i < 4; i++){
      printf("\n%s: <%p>: hpdpt[%d]=0x%lx",__FUNCTION__, &hpdpt[i], i, hpdpt[i]);
      tmp = pae_get_addr_from_pdpe(hpdpt[i]);
      hpd = (pdt_t)spa2hva1((u32)tmp); 
      for (j = 0; j < 512; j++){
          if(hpd[j]!=0){
              if(hpd[j] & _PAGE_PSE){
                printf("\n%s: <%p>: Big-Page hpd[%d]=0x%lx",__FUNCTION__, &hpd[j], j, hpd[j]);
              }
              else{
                printf("\n%s: <%p>: hpd[%d]=0x%lx",__FUNCTION__, &hpd[j], j, hpd[j]);
                tmp = hpd[j] & ~(PAGE_SIZE_4K-1);
                hpt = (pt_t) spa2hva1(tmp);
                for(z = 0; z < 512; z++){
                  if((hpt[z] & ~(PAGE_SIZE_4K-1))!=0) continue;
                    //printf("\n%s: <%p>: hpt[%d]=0x%lx",__FUNCTION__, &hpt[z], z, hpt[z]);
                }
              }  
          }
      }
    }
}

/* Function to print nodes in a given linked list */
void print_list(struct proc_map_page_4K *start)
{
    u32 counter=0;
    proc_map_page_4K *temp = start;
    printf("\n");
    while (temp->next!=NULL || temp->next!=0)
    { 
	while(counter <=100000);{
		counter++;
	}
	counter=0;
	if(!temp->is_huge_page){
        	printf("<virt> 0x%lx, <phys> 0x%lx\n", temp->vaddr, temp->paddr);
        }
	temp = temp->next;
    }
    /* the last entry is filled with 0, no need to print */
}
