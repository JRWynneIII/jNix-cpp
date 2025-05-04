.global swap_to_context
.global swap_to_kernel_context
.global save_cpu_context
.global enable_sysret

.macro swap_gs
	cmp $0x08, 0x8(%rsp)
	je 1f
	swapgs
1:
.endm

//Args: cpu_context_t* context: location of struct to save context into
save_cpu_context:
	movq %r15, (%rdi)
	movq %r14, 8(%rdi)
	movq %r13, 16(%rdi)
	movq %r12, 24(%rdi)
	movq %r11, 32(%rdi)
	movq %r10, 40(%rdi)
	movq %r9, 48(%rdi)
	movq %r8, 56(%rdi)

	movq %rbp, 64(%rdi)
	movq %rdx, 88(%rdi)
	movq %rcx, 96(%rdi)
	movq %rbx, 104(%rdi)
	movq %rax, 112(%rdi)

	movq %rsi, 80(%rdi)
	movq %rsp, 144(%rdi)

	//TODO: save rflags
	//pushf then pop to address

	ret

//Args: cpu_context_t* context: Location of context to restore
//	uint64_t ring: either 0 or 3, used to set the segment regs
enable_sysret:
	mov $0xc0000082, %rcx
	wrmsr
	mov $0xc0000080, %rcx
	rdmsr
	or $1, %eax
	wrmsr
	mov $0xc0000081, %rcx
	rdmsr
	mov $0x00180008, %edx
	wrmsr
	ret

swap_to_kernel_context:
	movq (%rdi), %r15
	movq 8(%rdi), %r14
	movq 16(%rdi), %r13
	movq 24(%rdi), %r12
	movq 32(%rdi), %r11
	movq 40(%rdi), %r10
	movq 48(%rdi), %r9
	movq 56(%rdi), %r8

	//movq 64(%rdi), %rbp
	movq 88(%rdi), %rdx
	movq 96(%rdi), %rcx
	movq 104(%rdi), %rbx
	movq 112(%rdi), %rax

	push %rsp
	push 152(%rdi) //ss
	push 144(%rdi) //rsp
	//push 136(%rdi) //rflags
	pushf
	push 128(%rdi) //cs
	push 120(%rdi) //rip


	//Saving rsi and rdi for last because...well we need them
	movq 80(%rdi), %rsi
	//movq 72(%rdi), %rdi

	iretq

swap_to_context:
	movq (%rdi), %r15
	movq 8(%rdi), %r14
	movq 16(%rdi), %r13
	movq 24(%rdi), %r12
	movq 32(%rdi), %r11
	movq 40(%rdi), %r10
	movq 48(%rdi), %r9
	movq 56(%rdi), %r8

	//movq 64(%rdi), %rbp
	movq 88(%rdi), %rdx
	movq 96(%rdi), %rcx
	movq 104(%rdi), %rbx
	movq 112(%rdi), %rax


	//Testing a 'stack frame' for when an interrupt gets called?
	push $0x8
	pushf
	push %rsp
	push $0x10

	//push 120(%rdi) //rip
	//push 128(%rdi) //cs
	//push 136(%rdi) //rflags
	//push 144(%rdi) //rsp
	//push 152(%rdi) //ss

	//Saving rsi and rdi for last because...well we need them
	movq 80(%rdi), %rsi
	//movq 72(%rdi), %rdi

	//iretq

	//sysret version:
	//Move entrypoint into ecx
	mov 120(%rdi), %ecx
	//Move rflags into r11
	mov $0x202, %r11
	mov 144(%rdi), %rsp
	sysretq

//OLD/IRETQ based method...does not work.
//	cli
//	movq (%rdi), %rax
//	movq 8(%rdi), %rcx
//	movq 16(%rdi), %rdx
//	movq 24(%rdi), %rbx
//	//movq 40(%rdi), %rbp
//	movq 72(%rdi), %r8
//	movq 80(%rdi), %r9
//	movq 88(%rdi), %r10
//	movq 96(%rdi), %r11
//	movq 104(%rdi), %r12
//	movq 112(%rdi), %r13
//	movq 120(%rdi), %r14
//	movq 128(%rdi), %r15
////	movw 144(%rdi), %cs
////	movw 148(%rdi), %ss
////	movw 152(%rdi), %ds
////	movw 156(%rdi), %es
////	movw 160(%rdi), %fs
////	movw 164(%rdi), %gs
//
//
//	//Push current stack pointer
//	push %rsp
//
//	//Push DS push $0x20 | %rsi
//	pushq %rax
//	movq $0x20, %rax
//	orq %rsi, %rax // TODO: check operand order!
//	xchgq (%rsp), %rax
//	
//	//Push the new stack pointer
//	pushq 32(%rdi)
//
//	//Push RFLAGS
//	pushf
//	
//	//Push CS push $0x18 | %rsi
//	pushq %rax
//	movq $0x18, %rax
//	orq %rsi, %rax
//	xchgq (%rsp), %rax
//	
//	//Push rip
//	pushq 64(%rdi)
//
//	//Overwriting rsi and rdi here at the end, so that we preserve our function args
//	movq 48(%rdi), %rsi
//	movq 56(%rdi), %rdi
//
//	iretq



//TODO: Handle ring 3
// rdi == new stack pointer
// rsi == new entry point
//.global exit_to_ring3
//exit_to_ring3:
//	cli
//
//	//push %rip
//	push %rsp
//	//Push SS since iretq handles this
//	push $0x23
//	//Push new stack pointer
//	push %rdi
//	//pushf
//	push $0x202
//	//push CS
//	push $0x1B
//	//Push the entry point/new RIP
//	push %rsi
//	iretq
