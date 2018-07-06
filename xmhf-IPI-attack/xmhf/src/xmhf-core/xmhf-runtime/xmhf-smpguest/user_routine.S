    .global switch_to_user_space
 switch_to_user_space:
    cli
    movl 0x4(%esp), %ebx
    movl 0x8(%esp), %ecx
    movl 0xc(%esp), %esi
    movl 0x10(%esp), %ebp
    movl $0x23, %ax
    movl %ax, %ds      
    movl %ax, %es      
    movl %ax, %fs      
    pushl $0x23
    pushl %ebx
    pushfl
    # popl %eax
    # and $0xbff, %eax
    # pushl %eax
    pushl $0x1B
    pushl %ecx
    iretl
    
    .global gdt_flush
gdt_flush:
    movl 0x4(%esp), %eax
    movl %eax, %ecx
    lgdt (%eax)       
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw %ax, %fs
    movw %ax, %gs
    jmpl $0x08,$(flush)

flush:
    ret

    .global tss_flush
tss_flush:
    movl $0x2B, %eax
    ltr %ax
    ret