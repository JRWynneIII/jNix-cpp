.section .bss
.align 16
.global stack_top
stack_bottom:
	.skip 0x100000
stack_top:

.section .text
.global _start
_start:
	mov $stack_top, %rsp
	call kmain
	cli
	hlt

.size _start, . - _start
