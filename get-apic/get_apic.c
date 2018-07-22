#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/apic.h>
#include <linux/cpu.h>
#include <linux/highmem.h>
#include <asm/msr.h>
#include <asm/mtrr.h>
#include <asm/ipi.h>
#include <linux/delay.h>
MODULE_LICENSE("GPL");

static int mm_exp_load(void){
	u64 msr;
	unsigned int cfg;	
	int count=0;
	if(!cpu_has_x2apic){
		printk("CPU has no APIC\n");	
	}
	rdmsrl(MSR_IA32_APICBASE,msr);
	printk("MSR_APICBASE = 0x%llx\n", msr);
	if (msr & X2APIC_ENABLE){
		printk("X2APIC is enabled\n");	
	}
	else {
		printk("X2APIC is disabled\n");
	}
	printk("Core apic_id = %d\n", read_apic_id());
	printk("Core id = %d\n", smp_processor_id());

	// safe_apic_wait_icr_idle();
	// cfg = SET_APIC_DEST_FIELD(0);
	// printk("cfg = 0x%lx\n", cfg);
	// *(volatile u32 *)(APIC_BASE+APIC_ICR2)=cfg;
	
	// cfg = 0|0x400;
	// cfg = 0|0x80;
 	//    cfg = 0|0x7F00;
	// printk("cfg = 0x%lx\n", cfg);
	 while(count<1){
	 		apic_wait_icr_idle();
			mdelay(2000);
	 		cfg = __prepare_ICR2(8);
	 		printk("cfg=0x%lx\n", cfg);
 	   		*(volatile u32 *)(APIC_BASE+APIC_ICR2)=cfg;
	 		cfg = __prepare_ICR(APIC_DEST_ALLBUT,NMI_VECTOR,APIC_DEST_LOGICAL);
	 		*(volatile u32 *)(APIC_BASE+APIC_ICR)=cfg;
	 		count++;
	 		printk("Count=%d\n",count);
	 }
	// printk("sent\n");
	//while(count<10){
	//	        __default_send_IPI_shortcut(APIC_DEST_ALLBUT, NMI_VECTOR,APIC_DEST_LOGICAL);
	//		count++;
	//}
	printk("end\n");

	return 0;
}

static void mm_exp_unload(void){
	return;
}

module_init(mm_exp_load);
module_exit(mm_exp_unload);
