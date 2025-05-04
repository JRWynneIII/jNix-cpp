.global _start

.extern main
.extern syscall_exit

_start:
	//Argc
	//pop %rdi
	////Set up ptr to argv
	//movq %rsp, %rsi
	movq $0x1, %rdi	//testing
	
	call main

	//mov $23, %rax	//Syscall number for SYS_EXIT
	mov $0, %rdi	//TODO: Change to return code

	//syscall
	//TODO: Make a syscall here to end the process

	ret
