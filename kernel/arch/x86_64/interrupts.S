.section .text
.align 8

.macro swap_gs
	cmp $0x08, 0x8(%rsp)
	je 1f
	swapgs
1:
.endm

.macro pushall
	cld
	push %rax
	push %rbx
	push %rcx
	push %rdx
	push %rsi
	push %rdi
	push %rbp
	push %r8
	push %r9
	push %r10
	push %r11
	push %r12
	push %r13
	push %r14
	push %r15
.endm

.macro popall
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rbp
	pop %rdi
	pop %rsi
	pop %rdx
	pop %rcx
	pop %rbx
	pop %rax
.endm

//TODO: Stub out all the isrs and irqs to use a macro to clean this shit up


.global isr0
.global isr1
.global isr2
.global isr3
.global isr4
.global isr5
.global isr6
.global isr7
.global isr8
.global isr9
.global isr10
.global isr11
.global isr12
.global isr13
.global isr14
.global isr15
.global isr16
.global isr17
.global isr18
.global isr19
.global isr20
.global isr21
.global isr22
.global isr23
.global isr24
.global isr25
.global isr26
.global isr27
.global isr28
.global isr29
.global isr30
.global isr31

.global irq0
.global irq1
.global irq2
.global irq8

#  0: Divide By Zero Exception
isr0: 
	cli
	pushq $0
	pushq $0
	jmp isr_stub

#  1: Debug Exception
isr1: 
	cli
	pushq $0
	pushq $1
	jmp isr_stub

# 2: Non Maskable Interrupt Exception
isr2:
	cli
	pushq $0
	pushq $2
	jmp isr_stub

# 3: Breakpoint Exception
isr3:
	cli
	pushq $0
	pushq $3
	jmp isr_stub

# 4: Into detected overflow exception
isr4:
	cli
	pushq $0
	pushq $4
	jmp isr_stub

# 5: Out of Bounds exception
isr5:
	cli
	pushq $0
	pushq $5
	jmp isr_stub

# 6: Invalid Opcode exception
isr6:
	cli
	pushq $0
	pushq $6
	jmp isr_stub

# 7: No coprocessor exception
isr7:
	cli
	pushq $0
	pushq $6
	jmp isr_stub
	 
#  8: Double Fault Exception (With Error Code!)
isr8: 
	cli
	pushq $8		   # No error code is pushed because cpu does it for us
	jmp isr_stub


# 9: Coprocessor Segment overrun exceptin
isr9:
	cli
	pushq $0
	pushq $9
	jmp isr_stub

# 10: Bad TSS exception
isr10:
	cli
	pushq $10
	jmp isr_stub

# 11: Segment not present exception
isr11:
	cli
	pushq $11
	jmp isr_stub

# 12: Stack Fault exception
isr12:
	cli
	pushq $12
	jmp isr_stub

# 13: General Protection Fault exception
isr13:
	cli
	pushq $13
	jmp isr_stub

# 14: Page Fault exception
isr14:
	cli
	pushq $14
	jmp isr_stub

# 15: REserved
isr15:
	cli
	pushq $0
	pushq $15
	jmp isr_stub

# 16: Floating Point Exception
isr16:
	cli
	pushq $0
	pushq $16
	jmp isr_stub

# 17: Alignment Check Exception
isr17:
	cli
	pushq $0
	pushq $17
	jmp isr_stub

# 18: Machine Check Exception
isr18:
	cli
	pushq $0
	pushq $18
	jmp isr_stub

# 19: Reserved
isr19:
	cli
	pushq $0
	pushq $19
	jmp isr_stub


# 19: Reserved
isr20:
	cli
	pushq $0
	pushq $20
	jmp isr_stub

# 19: Reserved
isr21:
	cli
	pushq $0
	pushq $21
	jmp isr_stub

# 19: Reserved
isr22:
	cli
	pushq $0
	pushq $22
	jmp isr_stub

isr23:
	cli
	pushq $0
	pushq $23
	jmp isr_stub

isr24:
	cli
	pushq $0
	pushq $24
	jmp isr_stub

isr25:
	cli
	pushq $0
	pushq $25
	jmp isr_stub

isr26:
	cli
	pushq $0
	pushq $26
	jmp isr_stub

isr27:
	cli
	pushq $0
	pushq $27
	jmp isr_stub

isr28:
	cli
	pushq $0
	pushq $28
	jmp isr_stub

isr29:
	cli
	pushq $0
	pushq $29
	jmp isr_stub

isr30:
	cli
	pushq $0
	pushq $30
	jmp isr_stub

isr31:
	cli
	pushq $0
	pushq $31
	jmp isr_stub

.extern fault_handler
.type fault_handler, @function

#ISR common stub
isr_stub: 
	swap_gs
	pushall
	
	mov %rsp, %rdi 
	call fault_handler

	popall
	swap_gs

	add $16, %rsp
	iretq

irq0:
	cli
	pushq $0
	pushq $32
	jmp irq_stub

irq1:
	cli
	pushq $0
	pushq $33
	jmp irq_stub

irq2:
	cli
	pushq $0
	pushq $34
	jmp irq_stub

irq8:
	cli
	pushq $0
	pushq $40
	jmp irq_stub

.extern irq_handler
.type irq_handler, @function

#IRQ common stub
irq_stub:
	swap_gs
	pushall
	
	mov %rsp, %rdi 
	call irq_handler

	popall

	swap_gs
	add $16, %rsp
	iretq
