#pragma once

extern char* exception_messages[];
struct registers {
	uintptr_t r15, r14, r13, r12;
	uintptr_t r11, r10, r9, r8;
	uintptr_t rbp, rdi, rsi, rdx, rcx, rbx, rax;

	/* Pushed by wrapper */
	uintptr_t int_no, err_code;

	/* Pushed by interrupt */
	uintptr_t rip, cs, rflags, rsp, ss;
};

void kpanic(struct registers* r);
