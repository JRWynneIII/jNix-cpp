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

namespace FrameBuffer {
	void init();
}

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

namespace GDT {
	void init();
}
