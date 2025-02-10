#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct idt_entry {
	uint16_t base_lo;
	uint16_t sel;
	uint8_t zero;
	uint8_t flags;
	uint16_t base_hi;
}__attribute__((packed));

struct idt_ptr {
	uint16_t limit;
	void* base;
}__attribute__((packed));

//Our IDT
struct idt_entry idt[256];
struct idt_ptr idtptr;

void load_idt(idt_ptr* idt) {
	__asm volatile("lidt (%0)" : : "r" (idt) : "memory");
}

void idt_set_gate(uint8_t idx, uint64_t base, uint8_t selector, uint8_t flags) {
	idt_entry entry;
	entry.base_lo = (base & 0xFFFF);
	entry.sel = selector;
	entry.zero = 0;
	entry.flags = flags;
	entry.base_hi = ((base >> 16) & 0xFFFF);
	idt[idx] = entry;
}

void init_idt() {
	idtptr.limit = (sizeof(idt_entry) * 265) - 1;
	idtptr.base = &idt;
	memset(&idt, 0, sizeof(idt_entry) * 256);
	load_idt(&idtptr);
}

