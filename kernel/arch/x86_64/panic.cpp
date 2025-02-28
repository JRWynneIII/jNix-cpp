#include<cstdint>
#include<cstddef>
#include<stdlib.h>
#include<jnix.h>
#include<panic.h>
#include<string.h>
#include<stdio.h>

void kpanic(struct registers* r) {
	// Dump registers
	// Write the exception message
	logk("\n\n\n", NONE);
	logk(exception_messages[(uint64_t)r->int_no], PANIC);
	logk("\n============================================================================================================================\n", NONE);
	logk("Interrupt: ", NONE);
	logk(itoa(r->int_no), NONE);
	logk("\tError code: ", NONE);
	logk(itoa(r->err_code), NONE);
	logk("\nr15:\t", NONE);
	logk(hex_to_str(r->r15), NONE);
	logk("\tr14:\t", NONE);
	logk(hex_to_str(r->r14), NONE);
	logk("\tr13:\t", NONE);
	logk(hex_to_str(r->r13), NONE);
	logk("\tr12:\t", NONE);
	logk(hex_to_str(r->r12), NONE);
	logk("\nr11:\t", NONE);
	logk(hex_to_str(r->r11), NONE);
	logk("\tr10:\t", NONE);
	logk(hex_to_str(r->r10), NONE);
	logk("\tr9:\t", NONE);
	logk(hex_to_str(r->r9), NONE);
	logk("\tr8:\t", NONE);
	logk(hex_to_str(r->r8), NONE);
	logk("\nrbp:\t", NONE);
	logk(hex_to_str(r->rbp), NONE);
	logk("\trdi:\t", NONE);
	logk(hex_to_str(r->rdi), NONE);
	logk("\trsi:\t", NONE);
	logk(hex_to_str(r->rsi), NONE);
	logk("\trdx:\t", NONE);
	logk(hex_to_str(r->rdx), NONE);
	logk("\nrcx:\t", NONE);
	logk(hex_to_str(r->rcx), NONE);
	logk("\trbx:\t", NONE);
	logk(hex_to_str(r->rbx), NONE);
	logk("\trax:\t", NONE);
	logk(hex_to_str(r->rax), NONE);
	logk("\trip:\t", NONE);
	logk(hex_to_str(r->rip), NONE);
	logk("\ncs:\t", NONE);
	logk(hex_to_str(r->cs), NONE);
	logk("\trflags:\t", NONE);
	logk(hex_to_str(r->rflags), NONE);
	logk("\trsp:\t", NONE);
	logk(hex_to_str(r->rsp), NONE);
	logk("\tss:\t", NONE);
	logk(hex_to_str(r->ss), NONE);
	logk("\n============================================================================================================================\n", NONE);
	for (;;);
}
