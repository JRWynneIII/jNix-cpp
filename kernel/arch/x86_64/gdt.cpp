#include <kernel.h>

#define NUM_GDT_ENTRIES 6

typedef struct gdt_entry {
	uint16_t limit_lo;
	uint16_t base_lo;
	uint8_t base_mid;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_hi;
} __attribute__((packed)) gdt_entry;

typedef struct gdt_ptr {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed)) gdt_ptr;

gdt_entry gdt[NUM_GDT_ENTRIES];
gdt_ptr gdtr;

extern "C" void reloadsegs();

namespace GDT {
	void set_gate(int idx, uint64_t base, uint64_t limit, uint8_t access, uint8_t gran) {
		gdt[idx].limit_lo = (limit & 0xFFFF);
		gdt[idx].base_lo = (base & 0xFFFF);
		gdt[idx].base_mid = (base >> 16) & 0xFF;
		gdt[idx].base_hi = (base >> 24) & 0xFF;
		gdt[idx].granularity = ((limit >> 16) & 0x0F);
		gdt[idx].granularity |= (gran & 0xF0);
		gdt[idx].access = access;
	}

	void init() {
		asm volatile("cli");
		gdtr.limit = (sizeof(gdt_entry) * NUM_GDT_ENTRIES) - 1;
		gdtr.base = (uint64_t)&gdt;
		// NULL
		set_gate(0, 0, 0, 0, 0);
		// Kernel Code 64
		set_gate(1, 0, 0xFFFF, 0x9A, 0xA0);
		// Kernel Data 64
		set_gate(2, 0, 0xFFFF, 0x92, 0xCF);
		// User Code 64
		set_gate(3, 0, 0xFFFF, 0xFA, 0xA0);
		// User Data 64
		set_gate(4, 0, 0xFFFF, 0xF2, 0xCF);
		//TSS
		set_gate(5, 0, 0x0067, 0xE9, 0);

		asm volatile("lgdt %0" : : "m"(gdtr));
		reloadsegs();
		asm volatile("sti");
	}
}
