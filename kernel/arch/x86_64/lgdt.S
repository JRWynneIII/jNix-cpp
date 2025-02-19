.section .text
.align 8

.global reloadsegs

reloadsegs:
	pushq $8
	lea reloadCS, %rax
	pushq %rax
	lretq 

reloadCS:
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss
	ret
