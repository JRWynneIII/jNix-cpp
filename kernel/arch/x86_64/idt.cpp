#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <kernel.h>
#include <kernel/gdt.hpp>

typedef struct idt_entry {
	uint16_t base_lo	:16;
	uint16_t kernel_cs	:16; //GDT segment that gets loaded into the gs reg before calling isr

	uint8_t ist		:3;
	uint8_t reserved0	:5;
	uint8_t type		:4;
	uint8_t zero		:1;
	uint8_t dpl		:2;
	uint8_t present		:1;

	uint16_t base_mid	:16;
	uint32_t base_hi	:32;
	uint32_t reserved	:32;
	
}__attribute__((packed)) idt_entry;

typedef struct idtr {
	uint16_t limit;
	uint64_t base;
}__attribute__((packed)) idtr;

//Our IDT
__attribute__((aligned(0x10)))
static idt_entry idt[256];
static idtr idtptr;

extern "C" void load_idt();

namespace Interrupts {
	void idt_set_gate(uint8_t idx, void* base, uint8_t type) {
		//idt_entry* entry = &idt[idx];
		idt[idx].base_lo = (uint64_t)base & 0xFFFF;
		idt[idx].kernel_cs = GDT_KERNEL_CODE_OFFSET;
		idt[idx].ist = 0;
		idt[idx].reserved0 = 0;
		idt[idx].type = type;
		idt[idx].zero = 0;
		idt[idx].dpl = 3;
		idt[idx].present = 1;

		idt[idx].base_mid = ((uint64_t)base >> 16) & 0xFFFF;
		idt[idx].base_hi = ((uint64_t)base >> 32) & 0xFFFFFFFF;

		idt[idx].reserved = 0;
	}

	void clear_idt() {
		memset(&idt, 0, sizeof(idt_entry) * 256);
	}
	
	void init_idt() {
		idtptr.base = (uint64_t)&idt[0];
		idtptr.limit = (uint16_t)(sizeof(idt_entry) * 256) - 1;
	}

	void load_idt() {
		asm volatile ("lidt %0" : : "m"(idtptr));
	}

	void test() {
		int i = 1 / 0;
		printfk("%d", i);

		logfk(KERNEL, "Divided by 0\n");
	}
}
