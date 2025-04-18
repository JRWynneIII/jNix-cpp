#include<cstdint>
#include<cstddef>
#include<stdlib.h>
#include<kernel.h>
#include<kernel/panic.h>
#include<string.h>
#include<stdio.h>

void kpanic(char* msg) {
	printk("\n\n\n");
	printk("============================================================================================================================\n");
	logfk(PANIC, "%s\n", msg);
	printk("============================================================================================================================\n");
	halt();
}

void kpanic(struct registers* r) {
	// Dump registers
	// Write the exception message
	printk("\n\n\n");
	logfk(PANIC, "%s\n", exception_messages[(uint64_t)r->int_no]);
	printk("============================================================================================================================\n");
	printfk("Interrupt: %d\tError code: %d\n", r->int_no, r->err_code);
	printfk("r15:\t%x\tr14:\t%x\tr13:\t%x\tr12:\t%x\n", r->r15, r->r14, r->r13, r->r12);
	printfk("r11:\t%x\tr10:\t%x\tr9:\t%x\tr8:\t%x\n", r->r11, r->r10, r->r9, r->r8);
	printfk("rbp:\t%x\trdi:\t%x\trsi:\t%x\trdx:\t%x\n", r->rbp, r->rdi, r->rsi, r->rdx);
	printfk("rcx:\t%x\trbx:\t%x\trax:\t%x\trip:\t%x\n", r->rcx, r->rbx, r->rax, r->rip);
	printfk("cs:\t%x\trflags:\t%x\trsp:\t%x\tss:\t%x\n", r->cs, r->rflags, r->rsp, r->ss);
	printk("============================================================================================================================\n");
	halt();
}
