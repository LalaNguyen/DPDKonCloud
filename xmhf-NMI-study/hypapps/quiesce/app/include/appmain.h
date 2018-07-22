#include <xmhf.h>
#include <malloc.h>
#include <hptw_emhf.h>
#include <tv_log.h>

#define	QUIESCE_HYPERCALL		0x44550002

pagelist_t *app_pl = NULL;

//extern u8 reserved_array[] __attribute__((section(".reserved_data")));



 typedef struct proc_map {
  u32 start_addr;
  u32 end_addr;
  int is_huge_page;
  struct proc_map *next;
} proc_map_t;

typedef struct proc_map_page_4K{
  u32 vaddr;
  u32 paddr;
  int is_huge_page;
  struct proc_map_page_4K *next;
} proc_map_page_4K;


struct desc_ptr {
 unsigned short size;
 unsigned long address;
} __attribute__((packed)) ;

// // This structure contains the value of one GDT entry.
// // We use the attribute 'packed' to tell GCC not to change
// // any of the alignment in the structure.
// struct gdt_entry_struct
// {
//     u16 limit_low;           // The lower 16 bits of the limit.
//     u16 base_low;            // The lower 16 bits of the base.
//     u8  base_middle;         // The next 8 bits of the base.
//     u8  access;              // Access flags, determine what ring this segment can be used in.
//     u8  granularity;
//     u8  base_high;           // The last 8 bits of the base.
// } __attribute__((packed));

// typedef struct gdt_entry_struct gdt_entry_t;

// struct tss32 {
//   unsigned int link : 16;
//   unsigned int reserved1 : 16;
//   unsigned int esp0 : 32;
//   unsigned int ss0 : 16;
//   unsigned int reserved2 : 16;
//   unsigned int esp1 : 32;
//   unsigned int ss1 : 16;
//   unsigned int reserved3 : 16;
//   unsigned int esp2 : 32;
//   unsigned int ss2 : 16;
//   unsigned int reserved4 : 16;
//   unsigned int cr3 : 32;
//   unsigned int eip : 32;
//   unsigned int eflags : 32;
//   unsigned int eax : 32;
//   unsigned int ecx : 32;
//   unsigned int edx : 32;
//   unsigned int ebx : 32;
//   unsigned int esp : 32;
//   unsigned int ebp : 32;
//   unsigned int esi : 32;
//   unsigned int edi : 32;
//   unsigned int es : 16;
//   unsigned int reserved5 : 16;
//   unsigned int cs : 16;
//   unsigned int reserved6 : 16;
//   unsigned int ss : 16;
//   unsigned int reserved7 : 16;
//   unsigned int ds : 16;
//   unsigned int reserved8 : 16;
//   unsigned int fs : 16;
//   unsigned int reserved9 : 16;
//   unsigned int gs : 16;
//   unsigned int reserved10 : 16;
//   unsigned int ldt : 16;
//   unsigned int reserved11 : 16;
//   unsigned int t : 1;
//   unsigned int reserved12 : 15;
//   unsigned int iomap : 16;
// } __attribute__ ((packed));

// typedef struct tss32 tss_entry_t; 

// tss_entry_t user_tss;

// // GDT & IDT management
// extern void gdt_flush(u32);
// extern void tss_flush(void);

// void xmhf_reload_gdt(VCPU * vcpu);
void add_systemcall(void);

// Hypervisor Memory Management
proc_map_page_4K * get_proc_map_from_guest_app(VCPU *vcpu, struct regs *r);
int update_hypervisor_pt(VCPU *vcpu, struct proc_map_page_4K *start);


// Utilities
void bubbleSort(struct proc_map_page_4K *start);
void swap(struct proc_map_page_4K *a, struct proc_map_page_4K *b);
void copy_from_gva_to_hva(VCPU *vcpu, gva_t vaddr, void *dst, u32 len);
void push_pa_map(proc_map_page_4K **, u32, u32, u32);

// Address Conversions
u64 gva2gpa(VCPU *vcpu, gva_t vaddr);
u64 get_hpa_from_hva(u32 vaddr);

// Debug Functions
void print_bytes (gva_t* start, int len);
void print_host_table(void);
void print_guest_table(gva_t);
