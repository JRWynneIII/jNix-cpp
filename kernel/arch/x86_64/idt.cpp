#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <kernel.h>

typedef struct idt_entry {
	uint16_t base_lo;
	uint16_t kernel_cs; //GDT segment that gets loaded into the gs reg before calling isr
	uint8_t ist;
	uint8_t attributes;
	uint16_t base_mid;
	uint32_t base_hi;
	uint32_t reserved;
	
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
	void idt_set_gate(uint8_t idx, void* base, uint8_t flags) {
		//idt_entry* entry = &idt[idx];
		idt[idx].base_lo = (uint64_t)base & 0xFFFF;
		// TODO: change this to the 64-bit kernel code entry in the GDT when you change that
		idt[idx].kernel_cs = 8;
		idt[idx].ist = 0;
		idt[idx].attributes = flags;
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
		logk(itoa(i), NONE);

		logk("Divided by 0\n", USER);
	}
}
