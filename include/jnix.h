#pragma once
#include<stdint.h>

void init_idt();
void printk(char* msg);

enum LOGLEVEL {
	KERNEL,
	USER,
	INFO,
	ERROR,
	PANIC,
	NONE
};

void logk(char* msg, enum LOGLEVEL level);
void init_framebuf();
void init_term();
unsigned char inportb (unsigned short _port);
void outportb (unsigned short _port, unsigned char _data);
//void init_gdt();
namespace Interrupts {
	void init();
	void init_idt();
	void load_idt();
	void idt_set_gate(uint8_t idx, void* base, uint8_t flags);
	void clear_idt();
	void test();
}

typedef struct mem_region;

struct mem_region {
	uint64_t idx;
	uintptr_t base;
	uintptr_t length;
	mem_region* next;
}; 

namespace Memory {
	void log_memory_info();
	void init_memmap();
	extern uint64_t virt_addr_offset;
	extern uint64_t hhdm_offset;
	extern mem_region usable_memory_regions[7];
	namespace Paging {
		void init();
		void test();
	}
}

namespace GDT {
	void init();
}
