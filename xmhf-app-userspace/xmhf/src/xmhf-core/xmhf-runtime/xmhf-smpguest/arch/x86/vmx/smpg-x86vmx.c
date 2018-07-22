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

// smpg-x86vmx - EMHF SMP guest component x86 (VMX) backend
// implementation
// author: amit vasudevan (amitvasudevan@acm.org)
#include <xmhf.h> 

/* Debug structs for GDT and IDT */
struct desc_ptr {
 unsigned short size;
 unsigned long address;
} __attribute__((packed)) ;

// This structure contains the value of one GDT entry.
// We use the attribute 'packed' to tell GCC not to change
// any of the alignment in the structure.
struct gdt_entry_struct
{
    u16 limit_low;           // The lower 16 bits of the limit.
    u16 base_low;            // The lower 16 bits of the base.
    u8  base_middle;         // The next 8 bits of the base.
    u8  access;              // Access flags, determine what ring this segment can be used in.
    u8  granularity;
    u8  base_high;           // The last 8 bits of the base.
} __attribute__((packed));

typedef struct gdt_entry_struct gdt_entry_t;

extern u64 x_my_gdt_start[] __attribute__(( section(".data") )); 
extern void switch_to_user_space(u32, u32, u32, u32); 

struct shared_gpr_struct
{
	u32 ebp;
        u32 esi;
	u32 eip;
        u32 esp;
} __attribute__((packed));

typedef struct shared_gpr_struct gpr_entry_t;

gpr_entry_t master_gpr;

// GDT & IDT management
extern void gdt_flush(u32);
extern void tss_flush(void);


void dump_xmhf_gdt_debug(u32 gdt_base, u32 size);
void dump_xmhf_idt_debug(u32 idt_base, u32 size);

static inline void load_gdt_remote_core(struct desc_ptr *dtr);
static inline void store_gdt_remote_core(struct desc_ptr *dtr);
static inline void store_idt_remote_core(struct desc_ptr *dtr);
static inline void load_idt_remote_core(struct desc_ptr *dtr);

void reload_idt_remote_core(VCPU *vcpu);

struct tss32 {
  unsigned int link : 16;
  unsigned int reserved1 : 16;
  unsigned int esp0 : 32;
  unsigned int ss0 : 16;
  unsigned int reserved2 : 16;
  unsigned int esp1 : 32;
  unsigned int ss1 : 16;
  unsigned int reserved3 : 16;
  unsigned int esp2 : 32;
  unsigned int ss2 : 16;
  unsigned int reserved4 : 16;
  unsigned int cr3 : 32;
  unsigned int eip : 32;
  unsigned int eflags : 32;
  unsigned int eax : 32;
  unsigned int ecx : 32;
  unsigned int edx : 32;
  unsigned int ebx : 32;
  unsigned int esp : 32;
  unsigned int ebp : 32;
  unsigned int esi : 32;
  unsigned int edi : 32;
  unsigned int es : 16;
  unsigned int reserved5 : 16;
  unsigned int cs : 16;
  unsigned int reserved6 : 16;
  unsigned int ss : 16;
  unsigned int reserved7 : 16;
  unsigned int ds : 16;
  unsigned int reserved8 : 16;
  unsigned int fs : 16;
  unsigned int reserved9 : 16;
  unsigned int gs : 16;
  unsigned int reserved10 : 16;
  unsigned int ldt : 16;
  unsigned int reserved11 : 16;
  unsigned int t : 1;
  unsigned int reserved12 : 15;
  unsigned int iomap : 16;
} __attribute__ ((packed));

typedef struct tss32 tss_entry_t; 

tss_entry_t user_tss;

/* End structs */


//the LAPIC register that is being accessed during emulation
static u32 g_vmx_lapic_reg __attribute__(( section(".data") )) = 0;

//the LAPIC operation that is being performed during emulation
static u32 g_vmx_lapic_op __attribute__(( section(".data") )) = LAPIC_OP_RSVD;

//guest TF and IF bit values during LAPIC emulation
static u32 g_vmx_lapic_guest_eflags_tfifmask __attribute__(( section(".data") )) = 0;	 

//----------------------------------------------------------------------
//vmx_lapic_changemapping
//change LAPIC mappings to handle SMP guest bootup

#define VMX_LAPIC_MAP			((u64)EPT_PROT_READ | (u64)EPT_PROT_WRITE)
#define VMX_LAPIC_UNMAP			0

static void vmx_lapic_changemapping(VCPU *vcpu, u32 lapic_paddr, u32 new_lapic_paddr, u64 mapflag){
#ifndef __XMHF_VERIFICATION__
  u64 *pts;
  u32 lapic_page;
  u64 value;
  
  pts = (u64 *)vcpu->vmx_vaddr_ept_p_tables;
  lapic_page=lapic_paddr/PAGE_SIZE_4K;
  value = (u64)new_lapic_paddr | mapflag;
  
  pts[lapic_page] = value;

  xmhf_memprot_arch_x86vmx_flushmappings(vcpu);
#endif //__XMHF_VERIFICATION__
}
//----------------------------------------------------------------------


//---checks if all logical cores have received SIPI
//returns 1 if yes, 0 if no
static u32 have_all_cores_recievedSIPI(void){
  u32 i;
  VCPU *vcpu;
  
	//iterate through all logical processors in master-id table
	for(i=0; i < g_midtable_numentries; i++){
  	vcpu = (VCPU *)g_midtable[i].vcpu_vaddr_ptr;
		if(vcpu->isbsp)
			continue;	//BSP does not receive SIPI
		
		if(!vcpu->sipireceived)
			return 0;	//one or more logical cores have not received SIPI
  }
  
  return 1;	//all logical cores have received SIPI
}

//---SIPI processing logic------------------------------------------------------
//return 1 if lapic interception has to be discontinued, typically after
//all aps have received their SIPI, else 0
static u32 processSIPI(VCPU *vcpu, u32 icr_low_value, u32 icr_high_value){
  //we assume that destination is always physical and 
  //specified via top 8 bits of icr_high_value
  u32 dest_lapic_id;
  VCPU *dest_vcpu = (VCPU *)0;
  
  HALT_ON_ERRORCOND( (icr_low_value & 0x000C0000) == 0x0 );
  
  dest_lapic_id= icr_high_value >> 24;
  
  printf("\nCPU(0x%02x): %s, dest_lapic_id is 0x%02x", 
		vcpu->id, __FUNCTION__, dest_lapic_id);
  
  //find the vcpu entry of the core with dest_lapic_id
  {
    int i;
    for(i=0; i < (int)g_midtable_numentries; i++){
      if(g_midtable[i].cpu_lapic_id == dest_lapic_id){
        dest_vcpu = (VCPU *)g_midtable[i].vcpu_vaddr_ptr;
        HALT_ON_ERRORCOND( dest_vcpu->id == dest_lapic_id );
        break;        
      }
    }
    
    HALT_ON_ERRORCOND( dest_vcpu != (VCPU *)0 );
  }

  printf("\nCPU(0x%02x): found AP to pass SIPI; id=0x%02x, vcpu=0x%08x",
      vcpu->id, dest_vcpu->id, (u32)dest_vcpu);  
  
  
  //send the sipireceived flag to trigger the AP to start the HVM
  if(dest_vcpu->sipireceived){
    printf("\nCPU(0x%02x): destination CPU #0x%02x has already received SIPI, ignoring", vcpu->id, dest_vcpu->id);
  }else{
		dest_vcpu->sipivector = (u8)icr_low_value;
  	dest_vcpu->sipireceived = 1;
  	printf("\nCPU(0x%02x): Sent SIPI command to AP, should awaken it!",
               vcpu->id);
  }

	if(have_all_cores_recievedSIPI())
		return 1;	//all cores have received SIPI, we can discontinue LAPIC interception
	else
		return 0;	//some cores are still to receive SIPI, continue LAPIC interception  
}



//----------------------------------------------------------------------
//xmhf_smpguest_arch_x86vmx_initialize
//initialize LAPIC interception machinery
//note: called from the BSP
void xmhf_smpguest_arch_x86vmx_initialize(VCPU *vcpu){
  u32 eax, edx;

  //read LAPIC base address from MSR
  rdmsr(MSR_APIC_BASE, &eax, &edx);
  HALT_ON_ERRORCOND( edx == 0 ); //APIC should be below 4G

  g_vmx_lapic_base = eax & 0xFFFFF000UL;
  //printf("\nBSP(0x%02x): LAPIC base=0x%08x", vcpu->id, g_vmx_lapic_base);
  
  //unmap LAPIC page
  vmx_lapic_changemapping(vcpu, g_vmx_lapic_base, g_vmx_lapic_base, VMX_LAPIC_UNMAP);
}
//----------------------------------------------------------------------





#ifdef __XMHF_VERIFICATION_DRIVEASSERTS__
	bool g_vmx_lapic_npf_verification_guesttrapping = false;
	bool g_vmx_lapic_npf_verification_pre = false;
#endif
//----------------------------------------------------------------------
//xmhf_smpguest_arch_x86vmx_eventhandler_hwpgtblviolation
//handle LAPIC accesses by the guest, used for SMP guest boot
u32 xmhf_smpguest_arch_x86vmx_eventhandler_hwpgtblviolation(VCPU *vcpu, u32 paddr, u32 errorcode){

  //get LAPIC register being accessed
  g_vmx_lapic_reg = (paddr - g_vmx_lapic_base);

#ifdef __XMHF_VERIFICATION_DRIVEASSERTS__
  g_vmx_lapic_npf_verification_pre = (errorcode & EPT_ERRORCODE_WRITE) &&
	((g_vmx_lapic_reg == LAPIC_ICR_LOW) || (g_vmx_lapic_reg == LAPIC_ICR_HIGH));
#endif


	if(errorcode & EPT_ERRORCODE_WRITE){			//LAPIC write

		if(g_vmx_lapic_reg == LAPIC_ICR_LOW || g_vmx_lapic_reg == LAPIC_ICR_HIGH ){
			g_vmx_lapic_op = LAPIC_OP_WRITE;
			vmx_lapic_changemapping(vcpu, g_vmx_lapic_base, hva2spa1(&g_vmx_virtual_LAPIC_base), VMX_LAPIC_MAP);
		}else{
			g_vmx_lapic_op = LAPIC_OP_RSVD;
			vmx_lapic_changemapping(vcpu, g_vmx_lapic_base, g_vmx_lapic_base, VMX_LAPIC_MAP);
		}    
	
	}else{											//LAPIC read
		if(g_vmx_lapic_reg == LAPIC_ICR_LOW || g_vmx_lapic_reg == LAPIC_ICR_HIGH ){
			g_vmx_lapic_op = LAPIC_OP_READ;
			vmx_lapic_changemapping(vcpu, g_vmx_lapic_base, hva2spa1(&g_vmx_virtual_LAPIC_base), VMX_LAPIC_MAP);
		}else{
			g_vmx_lapic_op = LAPIC_OP_RSVD;
			vmx_lapic_changemapping(vcpu, g_vmx_lapic_base, g_vmx_lapic_base, VMX_LAPIC_MAP);
		}  
	}

  //setup #DB intercept 
  vcpu->vmcs.control_exception_bitmap |= (1UL << 1); //enable INT 1 intercept (#DB fault)
  
  //save guest IF and TF masks
  g_vmx_lapic_guest_eflags_tfifmask = (u32)vcpu->vmcs.guest_RFLAGS & ((u32)EFLAGS_IF | (u32)EFLAGS_TF);	

  //set guest TF
  vcpu->vmcs.guest_RFLAGS |= EFLAGS_TF;

  #ifdef __XMHF_VERIFICATION_DRIVEASSERTS__
	g_vmx_lapic_npf_verification_guesttrapping = true;
  #endif
	
  //disable interrupts by clearing guest IF on this CPU until we get 
  //control in lapic_access_dbexception after a DB exception
  vcpu->vmcs.guest_RFLAGS &= ~(EFLAGS_IF);

#ifdef __XMHF_VERIFICATION_DRIVEASSERTS__
  assert(!g_vmx_lapic_npf_verification_pre || g_vmx_lapic_npf_verification_guesttrapping);
#endif

#ifdef __XMHF_VERIFICATION_DRIVEASSERTS__
  assert ( ((g_vmx_lapic_op == LAPIC_OP_RSVD) || 
					   (g_vmx_lapic_op == LAPIC_OP_READ) ||
					   (g_vmx_lapic_op == LAPIC_OP_WRITE))
					 );	

  assert ( ((g_vmx_lapic_reg >= 0) &&
					   (g_vmx_lapic_reg < PAGE_SIZE_4K))
					 );	
#endif

    return 0; // TODO: currently meaningless
}
//----------------------------------------------------------------------



#ifdef __XMHF_VERIFICATION_DRIVEASSERTS__
	bool g_vmx_lapic_db_verification_coreprotected = false;
	bool g_vmx_lapic_db_verification_pre = false;
#endif
//------------------------------------------------------------------------------
//xmhf_smpguest_arch_x86vmx_eventhandler_dbexception
//handle instruction that performed the LAPIC operation
void xmhf_smpguest_arch_x86vmx_eventhandler_dbexception(VCPU *vcpu, struct regs *r){
  u32 delink_lapic_interception=0;
  
  (void)r;

#ifdef	__XMHF_VERIFICATION_DRIVEASSERTS__
	//this handler relies on two global symbols apart from the parameters, set them 
	//to non-deterministic values with correct range
	//note: LAPIC #npf handler ensures this at runtime
	g_vmx_lapic_op = (nondet_u32() % 3) + 1;
	g_vmx_lapic_reg = (nondet_u32() % PAGE_SIZE_4K);
#endif


  if(g_vmx_lapic_op == LAPIC_OP_WRITE){			//LAPIC write
    u32 src_registeraddress, dst_registeraddress;
    u32 value_tobe_written;
    
    HALT_ON_ERRORCOND( (g_vmx_lapic_reg == LAPIC_ICR_LOW) || (g_vmx_lapic_reg == LAPIC_ICR_HIGH) );
   
    src_registeraddress = (u32)&g_vmx_virtual_LAPIC_base + g_vmx_lapic_reg;
    dst_registeraddress = (u32)g_vmx_lapic_base + g_vmx_lapic_reg;
    
	#ifdef __XMHF_VERIFICATION__
		//TODO: hardware modeling
		value_tobe_written= nondet_u32();
		#ifdef __XMHF_VERIFICATION_DRIVEASSERTS__
		g_vmx_lapic_db_verification_pre = (g_vmx_lapic_op == LAPIC_OP_WRITE) &&
		(g_vmx_lapic_reg == LAPIC_ICR_LOW) &&
		(((value_tobe_written & 0x00000F00) == 0x500) || ( (value_tobe_written & 0x00000F00) == 0x600 ));
		#endif
		
	#else
		value_tobe_written= *((u32 *)src_registeraddress);
	#endif

    
    if(g_vmx_lapic_reg == LAPIC_ICR_LOW){
      if ( (value_tobe_written & 0x00000F00) == 0x500){
        //this is an INIT IPI, we just void it
        printf("\n0x%04x:0x%08x -> (ICR=0x%08x write) INIT IPI detected and skipped, value=0x%08x", 
          (u16)vcpu->vmcs.guest_CS_selector, (u32)vcpu->vmcs.guest_RIP, g_vmx_lapic_reg, value_tobe_written);
        #ifdef __XMHF_VERIFICATION_DRIVEASSERTS__
			g_vmx_lapic_db_verification_coreprotected = true;
		#endif

      }else if( (value_tobe_written & 0x00000F00) == 0x600 ){
        //this is a STARTUP IPI
        u32 icr_value_high = *((u32 *)((u32)&g_vmx_virtual_LAPIC_base + (u32)LAPIC_ICR_HIGH));
        printf("\n0x%04x:0x%08x -> (ICR=0x%08x write) STARTUP IPI detected, value=0x%08x", 
          (u16)vcpu->vmcs.guest_CS_selector, (u32)vcpu->vmcs.guest_RIP, g_vmx_lapic_reg, value_tobe_written);        
		
		#ifdef __XMHF_VERIFICATION__
			#ifdef __XMHF_VERIFICATION_DRIVEASSERTS__
			g_vmx_lapic_db_verification_coreprotected = true;
			#endif
		#else
			delink_lapic_interception=processSIPI(vcpu, value_tobe_written, icr_value_high);
		#endif
      }else{
        #ifndef __XMHF_VERIFICATION__
			//neither an INIT or SIPI, just propagate this IPI to physical LAPIC
			*((u32 *)dst_registeraddress) = value_tobe_written;
		#endif //TODO: hardware modeling
      }
    }else{
       #ifndef __XMHF_VERIFICATION__
			*((u32 *)dst_registeraddress) = value_tobe_written;
	   #endif  //TODO: hardware modeling
    }
                
  }else if( g_vmx_lapic_op == LAPIC_OP_READ){		//LAPIC read
    u32 src_registeraddress;
    u32 value_read __attribute__((unused));
    HALT_ON_ERRORCOND( (g_vmx_lapic_reg == LAPIC_ICR_LOW) || (g_vmx_lapic_reg == LAPIC_ICR_HIGH) );

    src_registeraddress = (u32)&g_vmx_virtual_LAPIC_base + g_vmx_lapic_reg;
   
    //TODO: hardware modeling
    #ifndef __XMHF_VERIFICATION__
		value_read = *((u32 *)src_registeraddress);
	#else
		value_read = nondet_u32();
	#endif
  }

  //clear #DB intercept 
  vcpu->vmcs.control_exception_bitmap &= ~(1UL << 1); 

  //remove LAPIC interception if all cores have booted up
  if(delink_lapic_interception){
    printf("\n%s: delinking LAPIC interception since all cores have SIPI", __FUNCTION__);
	vmx_lapic_changemapping(vcpu, g_vmx_lapic_base, g_vmx_lapic_base, VMX_LAPIC_MAP);
  }else{
	vmx_lapic_changemapping(vcpu, g_vmx_lapic_base, g_vmx_lapic_base, VMX_LAPIC_UNMAP);
  }

  //restore guest IF and TF
  vcpu->vmcs.guest_RFLAGS &= ~(EFLAGS_IF);
  vcpu->vmcs.guest_RFLAGS &= ~(EFLAGS_TF);
  vcpu->vmcs.guest_RFLAGS |= g_vmx_lapic_guest_eflags_tfifmask;

#ifdef __XMHF_VERIFICATION_DRIVEASSERTS__
  assert(!g_vmx_lapic_db_verification_pre || g_vmx_lapic_db_verification_coreprotected);
#endif

}
//----------------------------------------------------------------------


//----------------------------------------------------------------------
//Queiscing interfaces
//----------------------------------------------------------------------

static void _vmx_send_quiesce_signal(VCPU __attribute__((unused)) *vcpu){
  volatile u32 *icr_low = (u32 *)(0xFEE00000 + 0x300);
  volatile u32 *icr_high = (u32 *)(0xFEE00000 + 0x310);
  u32 icr_high_value= 0xFFUL << 24;
  u32 prev_icr_high_value;
  u32 delivered;
  
  prev_icr_high_value = *icr_high;
  
  *icr_high = icr_high_value;    //send to all but self
  *icr_low = 0x000C0400UL;      //send NMI        
  
  //check if IPI has been delivered successfully
  //printf("\n%s: CPU(0x%02x): firing NMIs...", __FUNCTION__, vcpu->id);
#ifndef __XMHF_VERIFICATION__  
  do{
	delivered = *icr_high;
	delivered &= 0x00001000;
  }while(delivered);
#else
	//TODO: plug in h/w model of LAPIC, for now assume hardware just
	//works
#endif

  //restore icr high
  *icr_high = prev_icr_high_value;
    
  //printf("\n%s: CPU(0x%02x): NMIs fired!", __FUNCTION__, vcpu->id);
}


//quiesce interface to switch all guest cores into hypervisor mode
//note: we are in atomic processsing mode for this "vcpu"
void xmhf_smpguest_arch_x86vmx_quiesce(VCPU *vcpu){

        //printf("\nCPU(0x%02x): got quiesce signal...", vcpu->id);
        //grab hold of quiesce lock
        spin_lock(&g_vmx_lock_quiesce);
        //printf("\nCPU(0x%02x): grabbed quiesce lock.", vcpu->id);

		vcpu->quiesced = 1;
		//reset quiesce counter
        spin_lock(&g_vmx_lock_quiesce_counter);
        g_vmx_quiesce_counter=0;
        spin_unlock(&g_vmx_lock_quiesce_counter);
        
        //send all the other CPUs the quiesce signal
        g_vmx_quiesce=1;  //we are now processing quiesce
        _vmx_send_quiesce_signal(vcpu);
        
        //wait for all the remaining CPUs to quiesce
        //printf("\nCPU(0x%02x): waiting for other CPUs to respond...", vcpu->id);
        while(g_vmx_quiesce_counter < (g_midtable_numentries-1) );
        //printf("\nCPU(0x%02x): all CPUs quiesced successfully.", vcpu->id);

}
// Debug Functions
void print_bytes1 (gva_t* start, int len)
{
    unsigned int i = 0;
    for (; i < len / sizeof (gva_t); i ++)
    {
        printf ("%08lX ", start[i]);
        if ((i % 8) == 7) printf ("\n");
    }
}


u64 gva2gpa2(VCPU * vcpu, gva_t vaddr)
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


void my_xmhf_smpguest_arch_x86vmx_endquiesce(VCPU *vcpu,struct regs * r){
	unsigned int sleep, counter;
	uint64_t start, end;
	//int *dst;
	(void)vcpu;
        //set resume signal to resume the cores that are quiesced
        //Note: we do not need a spinlock for this since we are in any
        //case the only core active until this point
	printf("\nCPU(0x%02x): My EBP is 0x%lx...", vcpu->id, r->ebp);
        master_gpr.ebp=r->ebp;
        master_gpr.esi=r->esi;
	master_gpr.eip=vcpu->vmcs.guest_RIP;
        master_gpr.esp=vcpu->vmcs.guest_RSP;
	printf("\nCPU(0x%02x): My EIP is 0x%lx...", vcpu->id,vcpu->vmcs.guest_RIP);
        printf("\nCPU(0x%02x): My ESP is 0x%lx...", vcpu->id,vcpu->vmcs.guest_RSP);
        g_vmx_quiesce_resume_counter=0;
        printf("\nCPU(0x%02x): waiting for other CPUs to resume...", vcpu->id);
        /* First release for each core to update its GDT */
	printf("\nCPU(0x%02x): current resume counter: %d",vcpu->id,g_vmx_quiesce_resume_counter);
        // release core 0xc
	c0_vmx_quiesce_resume_signal=1;
        while(g_vmx_quiesce_resume_counter < 1);
        // release core 0xe
	c2_vmx_quiesce_resume_signal=1;
        while(g_vmx_quiesce_resume_counter < 2);
        // release core 0x1a
	c3_vmx_quiesce_resume_signal=1;
	while(g_vmx_quiesce_resume_counter < 3);
	// release core 0x1c
	c4_vmx_quiesce_resume_signal=1;
	while(g_vmx_quiesce_resume_counter < 4);
        // while(!g_vmx_quiesce_resume_counter);
        //while(g_vmx_quiesce_resume_counter < (g_midtable_numentries-1) );
        // release core 0x1e
	c5_vmx_quiesce_resume_signal=1;
	while(g_vmx_quiesce_resume_counter < 5);
        /* All cores have updated its GDT, Second release for each core to resume execution*/
        //c1_gdt_signal = 1;
        //while(g_vmx_quiesce_resume_counter < 4);
        c2_gdt_signal = 1;
        while(g_vmx_quiesce_resume_counter < 6);
        //c3_gdt_signal = 1;
        //while(g_vmx_quiesce_resume_counter < 6);

        g_vmx_quiesce_resume_counter = 8;
        vcpu->quiesced=0;
        g_vmx_quiesce=0;  // we are out of quiesce at this point

        printf("\nCPU(0x%02x): all CPUs resumed successfully.", vcpu->id);
        
        //release quiesce lock
        printf("\nCPU(0x%02x): releasing quiesce lock.", vcpu->id);
        spin_unlock(&g_vmx_lock_quiesce);
        sleep = 0;
   	counter = 0;
        start = rdtsc64();
	for(;;){

                if (sleep%1190494759==0){
                        printf("\nCPU(0x%02x): still alive with counter = %d.", vcpu->id, sleep);
                        counter++;
                }
                sleep++;
                if(counter==5) break;
        }
        end = rdtsc64();
	printf("\nCPU(0x%02x): start = %lld, end = %lld, gap = %lld", vcpu->id, start, end, end-start);
	printf("\nCPU(0x%02x): print forth byte at address 0x%lx:",vcpu->id,vcpu->vmcs.guest_RIP);
	/* modifying address of return instruction*/
	//dst = (int*)gpa2hva((u32)gva2gpa2(vcpu,(u32)vcpu->vmcs.guest_RIP+3));
	//*dst = 0xFEEB;
	//print_bytes1((gva_t*) dst, 10);
	
}

void reload_idt_remote_core(VCPU *vcpu){  struct desc_ptr idt_base; 
  // (void)vcpu;
  store_idt_remote_core(&idt_base); // get gdt base and size
  load_idt_remote_core(&idt_base);
  printf("\n CPU(0x%02x): new idt size %d", vcpu->id, idt_base.size+1);
  printf("\n CPU(0x%02x): new idt address 0x%lx", vcpu->id, idt_base.address);  dump_xmhf_idt_debug(idt_base.address, idt_base.size+1);
}

void reload_gdt_remote_core(VCPU *vcpu){
  struct desc_ptr gdt_base; 
  // (void)vcpu;
  store_gdt_remote_core(&gdt_base); // get gdt base and size
  load_gdt_remote_core(&gdt_base);
 /* Check newly loaded gdt */
  printf("\n CPU(0x%02x): new gdt size %d", vcpu->id, gdt_base.size+1);
  printf("\n CPU(0x%02x): new gdt address 0x%lx", vcpu->id, gdt_base.address);
  dump_xmhf_gdt_debug(gdt_base.address, gdt_base.size+1);
}

// Set the value of one GDT entry.
void gdt_set_gate(u32 * gdt_base, u32 num, u32 base, u32 limit, u8 access, u8 gran){
    gdt_entry_t * gdt_entries = (gdt_entry_t*) gdt_base;
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;
    
    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

static void write_tss(VCPU *vcpu, u32 * gdt_base, u32 num, u8 access, u8 limit){
    u32 tss_base;
    gdt_entry_t * gdt_entries;
    
    tss_base = (u32) &user_tss;
    gdt_entries = (gdt_entry_t*) gdt_base;

    gdt_entries[num].base_low       = (tss_base & 0xFFFF);
    gdt_entries[num].base_middle    = (tss_base >> 16) & 0xFF;
    gdt_entries[num].base_high      = (tss_base >> 24) & 0xFF;

    gdt_entries[num].limit_low      = sizeof(tss_entry_t);
    gdt_entries[num].access         = access;

    gdt_entries[num].granularity    = limit;

    // Ensure the TSS is initially zero'd.
    memset(&user_tss, 0, sizeof(tss_entry_t));

    // Set the kernel stack segment
    user_tss.ss0 = 0x10;
    // Set the kernel stack
    user_tss.esp0 = (u32) vcpu->esp;
    printf("\n%s: set ss0 to 0x%lx", __FUNCTION__, user_tss.ss0);
    printf("\n%s: set esp0 to 0x%lx", __FUNCTION__, user_tss.esp0);
}

static VCPU * _vmx_get_target_vcpu(unsigned int dest_id){
        int i;
        for (i=0; i<(int)g_midtable_numentries;i++){
                if(g_midtable[i].cpu_lapic_id == dest_id)
                        return( (VCPU*) g_midtable[i].vcpu_vaddr_ptr);
        }
        printf("\n%s: fatal, unable to retrieve vcpu for id=0x%02x", __FUNCTION__,dest_id);
        HALT();
        return NULL;
}


void update_gdt_remote_core(VCPU * vcpu){
  struct desc_ptr new_gdt; // gdt base addr and size
  u16 trselector =  0x2B;
  u16 gsselector =  0x33;
  VCPU *vcpu_dest = _vmx_get_target_vcpu(0xc);
  new_gdt.size = (sizeof(gdt_entry_t) * 9) - 1;
  new_gdt.address  = (u32)&x_my_gdt_start;
  
  gdt_set_gate((u32 *)new_gdt.address, 0, 0, 0, 0, 0);                // Null segment
  gdt_set_gate((u32 *)new_gdt.address, 1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
  gdt_set_gate((u32 *)new_gdt.address, 2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
  gdt_set_gate((u32 *)new_gdt.address, 3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
  gdt_set_gate((u32 *)new_gdt.address, 4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment
  write_tss(vcpu, (u32 *)new_gdt.address, 5,0xE9, 0x10);
  switch(vcpu->id){
        case 0xc:  
          gsselector = 0x33;
          gdt_set_gate((u32 *)new_gdt.address, 6, vcpu->vmcs.guest_GS_base, 0xFFFFFFFF, 0xF2, 0xDF); // User mode data segment
          break;
        case 0xe:
          gsselector = 0x3B;
          gdt_set_gate((u32 *)new_gdt.address, 7, vcpu_dest->vmcs.guest_GS_base, 0xFFFFFFFF, 0xF2, 0xDF); // User mode data segment
          break;
        case 0x1a:  
          gsselector = 0x43;
          gdt_set_gate((u32 *)new_gdt.address, 8, vcpu->vmcs.guest_GS_base, 0xFFFFFFFF, 0xF2, 0xDF); // User mode data segment
          break;
  }
  gdt_flush((u32)&new_gdt);
  
  /* Load new TR for user space */
  printf("\nCurrent TR: 0x%lx", read_tr_sel());
    asm volatile(
      "movw %0, %%ax\r\n"
      "ltr %%ax\r\n"
      :
      :"m"(trselector)
      :
    );
    printf("\nLoaded new TR: 0x%lx", read_tr_sel());
  /* Load new GS for user space */
  printf("\nCurrent GS: 0x%lx", read_segreg_gs());
    asm volatile(
      "mov %0, %%gs\r\n"
      :
      :"m"(gsselector)
      :
    );
    printf("\nLoaded new GS: 0x%lx", read_segreg_gs());
}

/*static VCPU * _vmx_get_target_vcpu(unsigned int dest_id){
	int i;
	for (i=0; i<(int)g_midtable_numentries;i++){
		if(g_midtable[i].cpu_lapic_id == dest_id)
			return( (VCPU*) g_midtable[i].vcpu_vaddr_ptr);
	}
	printf("\n%s: fatal, unable to retrieve vcpu for id=0x%02x", __FUNCTION__,dest_id);
	HALT();
	return NULL;
}*/
// My prepare context function
void prepare_context(VCPU *vcpu, struct regs *r){
  u32 guest_esp, guest_eip, guest_esi, guest_ebp;
  //unsigned int sleep;
  // u32 i, total_page, hva = 0;
  VCPU* vcpu_dest;
  int * p;
  uint64_t start,end;
  start = rdtsc64();
  //guest_ebp = r->ebp;
  //guest_esi = r->esi;
  //guest_esp = vcpu->vmcs.guest_RSP;
  //guest_eip = vcpu->vmcs.guest_RIP;
  //sleep = 0;
  vcpu_dest = _vmx_get_target_vcpu(0xa);
  printf("\nCPU(0x%02x): Address of VCPU 0x%02x (0xa) is %p",vcpu->id, vcpu_dest->id, vcpu_dest);

  vcpu_dest = _vmx_get_target_vcpu(0xc);
  printf("\nCPU(0x%02x): Address of VCPU 0x%02x (0xc) is %p",vcpu->id, vcpu_dest->id, vcpu_dest);
 	
  vcpu_dest = _vmx_get_target_vcpu(0xe);
  printf("\nCPU(0x%02x): Address of VCPU 0x%02x (0xe) is %p",vcpu->id, vcpu_dest->id, vcpu_dest);
  
  // use context of VCPI 0xc
  guest_ebp = master_gpr.ebp;
  guest_esi = master_gpr.esi;
  printf("\nCPU(0x%02x): My EBP is 0x%lx and target EBP is 0x%lx",vcpu->id,r->ebp,master_gpr.ebp);
  printf("\nCPU(0x%02x): My ESI is 0x%lx and target ESI is 0x%lx",vcpu->id,r->esi,master_gpr.esi);
  printf("\nCPU(0x%02x): My EIP is 0x%lx and target EIP is 0x%lx", vcpu->id, vcpu->vmcs.guest_RIP, master_gpr.eip);
  guest_esp = master_gpr.esp;
  guest_eip = master_gpr.eip+3;
  printf("\nCPU(0x%02x): vcpu_dest->vmcs.guest_RIP = 0x%lx", vcpu->id, vcpu_dest->vmcs.guest_RIP);
  printf("\nCPU(0x%02x): recorded guest RIP on VCPU (0x%02x) is 0x%lx", vcpu->id, vcpu_dest->id, master_gpr.eip);
  
  printf("\nCPU(0x%02x): My new EBP is 0x%lx",vcpu->id, guest_ebp);
  printf("\nCPU(0x%02x): My new ESI is 0x%lx",vcpu->id, guest_esi);
  printf("\nCPU(0x%02x): My new EIP is 0x%lx", vcpu->id, guest_eip);
  printf("\nCPU(0x%02x): My new ESP is 0x%lx", vcpu->id, guest_esp);

 

 
  end = rdtsc64();
  printf("\nCPU(0x%02x): Elapsed CPU cycle: %d", end-start);


  // xmhf_baseplatform_arch_x86vmx_dumpVMCS(vcpu);
  
  //for(;;){

  //              if (sleep%1190494759==0){
  //                      printf("\nCPU(0x%02x): still alive with counter = %d.", vcpu->id, sleep);
  //              }
  //              sleep++;
  //      }


  // Stage 1: User Land Jump 
  // 1.1 Check for GDT prepared by lcore 0
  update_gdt_remote_core(vcpu);
  printf("\n Reached end");
  g_vmx_quiesce_resume_counter++;

  if (vcpu->id == 0xc){
    while(!c1_gdt_signal);
  }
  else if (vcpu->id == 0xe){
    while(!c2_gdt_signal);
  }
  else if (vcpu->id == 0x1a){
    while(!c3_gdt_signal);
  }
  reload_gdt_remote_core(vcpu);
  reload_idt_remote_core(vcpu);
 //guest_eip = (u32)0x40000000;

  printf("\n CPU(0x%02x): Preparing context",vcpu->id); 
  printf("\n CPU(0x%02x): EBP of guest state : value = 0x%lx",vcpu->id, guest_ebp); 
  printf("\n CPU(0x%02x): ESI of guest state : value = 0x%lx",vcpu->id, guest_esi); 
  printf("\n CPU(0x%02x): RIP of guest state : value = 0x%lx",vcpu->id, guest_eip); 
  printf("\nCPU(0x%02x): EOQ received, resuming to host...", vcpu->id);
 
  g_vmx_quiesce_resume_counter++;

  /* simple user-space testing loop */
  p = (int*) 0x40000000;
  print_bytes1((gva_t*) p, 18);
  printf("\n");
  *p = 0xFEEB;
   print_bytes1 ((gva_t*)p, 18);
  guest_eip = (u32) 0x40000000;
  // total_page = 0xC0000000/PAGE_SIZE_4K;
  // for (i = 0x0; i<total_page; i++){
  //   TLB_INVLPG(guest_rip);
  //   hva += PAGE_SIZE_4K;  
  //   //printf("\n CPU(0x%02x): flushed 0x%lx",vcpu->id, hva); 
  // }
  while(g_vmx_quiesce_resume_counter<8);
  printf("\nCPU(0x%02x): Switch to user space and never return...", vcpu->id);
  switch_to_user_space(guest_esp, guest_eip, guest_esi,guest_ebp);

}

//quiescing handler for #NMI (non-maskable interrupt) exception event
//note: we are in atomic processsing mode for this "vcpu"
void xmhf_smpguest_arch_x86vmx_eventhandler_nmiexception(VCPU *vcpu, struct regs *r){
 u32 nmiinhvm; //1 if NMI originated from the HVM else 0 if within the hypervisor
 u32 _vmx_vmcs_info_vmexit_interrupt_information;
 u32 _vmx_vmcs_info_vmexit_reason;

    (void)r;

 //determine if the NMI originated within the HVM or within the
 //hypervisor. we use VMCS fields for this purpose. note that we
 //use vmread directly instead of relying on vcpu-> to avoid 
 //race conditions
 __vmx_vmread(0x4404, &_vmx_vmcs_info_vmexit_interrupt_information);
 __vmx_vmread(0x4402, &_vmx_vmcs_info_vmexit_reason);
  
 nmiinhvm = ( (_vmx_vmcs_info_vmexit_reason == VMX_VMEXIT_EXCEPTION) && ((_vmx_vmcs_info_vmexit_interrupt_information & INTR_INFO_VECTOR_MASK) == 2) ) ? 1 : 0;
  
 if(g_vmx_quiesce){ //if g_vmx_quiesce =1 process quiesce regardless of where NMI originated from
   //if this core has been quiesced, simply return
     if(vcpu->quiesced)
       return;
        
     vcpu->quiesced=1;
  
     //increment quiesce counter
     spin_lock(&g_vmx_lock_quiesce_counter);
     g_vmx_quiesce_counter++;
     spin_unlock(&g_vmx_lock_quiesce_counter);

     //wait until quiesceing is finished
     printf("\nCPU(0x%02x): Quiesced", vcpu->id);

     // Each vcpu has its own lock for signal

     switch(vcpu->id){
        case 0xa:  
          while(!c0_vmx_quiesce_resume_signal);
          break;
        case 0xc:  
          while(!c1_vmx_quiesce_resume_signal);
         // asm volatile ("pushfl \r\n"
         //               "popl %%eax\r\n"
         //               "or $0x200, %%eax\r\n"
         //               "pushl %%eax \r\n"
         //               "popfl\r\n":::"%eax");
         // prepare_context(vcpu, r);
          break;
        case 0xe:  
          while(!c2_vmx_quiesce_resume_signal);
          asm volatile ("pushfl \r\n"
                        "popl %%eax\r\n"
                        "or $0x200, %%eax\r\n"
                        "pushl %%eax \r\n"
                        "popfl\r\n":::"%eax");
          prepare_context(vcpu, r);
          break;
        case 0x1a:  
          while(!c3_vmx_quiesce_resume_signal);
         // asm volatile ("pushfl \r\n"
         //               "popl %%eax\r\n"
         //               "or $0x200, %%eax\r\n"
         //               "pushl %%eax \r\n"
         //               "popfl\r\n":::"%eax");
         // prepare_context(vcpu, r);
          break;
	case 0x1c:
	 	while(!c4_vmx_quiesce_resume_signal);
	 	break;
	case 0x1e:
		while(!c5_vmx_quiesce_resume_signal);
		break;
     }
     printf("\nCPU(0x%02x): EOQ received, resuming to guest...", vcpu->id);

     spin_lock(&g_vmx_lock_quiesce_resume_counter);
     g_vmx_quiesce_resume_counter++;
     spin_unlock(&g_vmx_lock_quiesce_resume_counter);
      
     vcpu->quiesced=0;
 }else{
   //we are not in quiesce
   //inject the NMI if it was triggered in guest mode
    
   if(nmiinhvm){
     if(vcpu->vmcs.control_exception_bitmap & CPU_EXCEPTION_NMI){
       //TODO: hypapp has chosen to intercept NMI so callback
     }else{
       printf("\nCPU(0x%02x): Regular NMI, injecting back to guest...", vcpu->id);
       vcpu->vmcs.control_VM_entry_exception_errorcode = 0;
       vcpu->vmcs.control_VM_entry_interruption_information = NMI_VECTOR |
         INTR_TYPE_NMI |
         INTR_INFO_VALID_MASK;
     }
   }
 }
  
}

//----------------------------------------------------------------------


//perform required setup after a guest awakens a new CPU
void xmhf_smpguest_arch_x86vmx_postCPUwakeup(VCPU *vcpu){
	//setup guest CS and EIP as specified by the SIPI vector
	vcpu->vmcs.guest_CS_selector = ((vcpu->sipivector * PAGE_SIZE_4K) >> 4); 
	vcpu->vmcs.guest_CS_base = (vcpu->sipivector * PAGE_SIZE_4K); 
	vcpu->vmcs.guest_RIP = 0x0ULL;
}

//walk guest page tables; returns pointer to corresponding guest physical address
//note: returns 0xFFFFFFFF if there is no mapping
u8 * xmhf_smpguest_arch_x86vmx_walk_pagetables(VCPU *vcpu, u32 vaddr){
  if((u32)vcpu->vmcs.guest_CR4 & CR4_PAE ){
    //PAE paging used by guest
    u32 kcr3 = (u32)vcpu->vmcs.guest_CR3;
    u32 pdpt_index, pd_index, pt_index, offset;
    u64 paddr;
    pdpt_t kpdpt;
    pdt_t kpd; 
    pt_t kpt; 
    u32 pdpt_entry, pd_entry, pt_entry;
    u32 tmp;

    // get fields from virtual addr 
    pdpt_index = pae_get_pdpt_index(vaddr);
    pd_index = pae_get_pdt_index(vaddr);
    pt_index = pae_get_pt_index(vaddr);
    offset = pae_get_offset_4K_page(vaddr);  

    //grab pdpt entry
    tmp = pae_get_addr_from_32bit_cr3(kcr3);
    kpdpt = (pdpt_t)((u32)tmp); 
    pdpt_entry = kpdpt[pdpt_index];
  
    //grab pd entry
    tmp = pae_get_addr_from_pdpe(pdpt_entry);
    kpd = (pdt_t)((u32)tmp); 
    pd_entry = kpd[pd_index];

    if ( (pd_entry & _PAGE_PSE) == 0 ) {
      // grab pt entry
      tmp = (u32)pae_get_addr_from_pde(pd_entry);
      kpt = (pt_t)((u32)tmp);  
      pt_entry  = kpt[pt_index];
      
      // find physical page base addr from page table entry 
      paddr = (u64)pae_get_addr_from_pte(pt_entry) + offset;
    }
    else { // 2MB page 
      offset = pae_get_offset_big(vaddr);
      paddr = (u64)pae_get_addr_from_pde_big(pd_entry);
      paddr += (u64)offset;
    }
  
    return (u8 *)(u32)paddr;
    
  }else{
    //non-PAE 2 level paging used by guest
    u32 kcr3 = (u32)vcpu->vmcs.guest_CR3;
    u32 pd_index, pt_index, offset;
    u64 paddr;
    npdt_t kpd; 
    npt_t kpt; 
    u32 pd_entry, pt_entry;
    u32 tmp;

    // get fields from virtual addr 
    pd_index = npae_get_pdt_index(vaddr);
    pt_index = npae_get_pt_index(vaddr);
    offset = npae_get_offset_4K_page(vaddr);  
  
    // grab pd entry
    tmp = npae_get_addr_from_32bit_cr3(kcr3);
    kpd = (npdt_t)((u32)tmp); 
    pd_entry = kpd[pd_index];
  
    if ( (pd_entry & _PAGE_PSE) == 0 ) {
      // grab pt entry
      tmp = (u32)npae_get_addr_from_pde(pd_entry);
      kpt = (npt_t)((u32)tmp);  
      pt_entry  = kpt[pt_index];
      
      // find physical page base addr from page table entry 
      paddr = (u64)npae_get_addr_from_pte(pt_entry) + offset;
    }
    else { // 4MB page 
      offset = npae_get_offset_big(vaddr);
      paddr = (u64)npae_get_addr_from_pde_big(pd_entry);
      paddr += (u64)offset;
    }
  
    return (u8 *)(u32)paddr;
  }
}

/* Debug function for checking context prepared by core 0 on core 2*/


static inline void load_gdt_remote_core(struct desc_ptr *dtr){
 asm volatile("lgdt %0":"=m" (*dtr));
}

static inline void store_idt_remote_core(struct desc_ptr *dtr){
 asm volatile("sidt %0":"=m" (*dtr));
}

static inline void store_gdt_remote_core(struct desc_ptr *dtr){
 asm volatile("sgdt %0":"=m" (*dtr));
}

static inline void load_idt_remote_core(struct desc_ptr *dtr){
 asm volatile("lidt %0":"=m" (*dtr));
}

void dump_xmhf_gdt_debug(u32 gdt_base, u32 size){
 u64 *gdt;
 u32 i;
 /* Dump current GDT information */
 printf("\n------Dumping GDT-----");
 gdt = (u64*)gdt_base;
 for (i = 0; i < (size/8); i++) {
   if(*(gdt+i)!=0 || i==0)
     printf("\n<0x%lx> GDT[%d]= 0x%llx", gdt+i, i, *(gdt+i));
 }
}

void dump_xmhf_idt_debug(u32 idt_base, u32 size){
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
