#pragma once
#include<kernel/gdt.hpp>
#include<kernel/tss.hpp>
#include<kernel/processor.hpp>

//We have to do this to basically 'template' the inline assembly. This sucks but eh
//#define READ_REG(r) asm volatile("mov %%" r ", %0" : "=r"(ret))
//#define WRITE_REG(r) asm volatile("mov %0, %%rax;" "mov %%rax, %%" r :: "g"(val) : "rax"); 

// TODO:
//To read/write registers, 
//	- create an enum of each register name
//	- add a switch/case for *each register* to the read_reg func where the reg name is hard coded

class x86_64 : public processor_t {
public:
	x86_64() {}
	~x86_64() {}
	virtual void load_gdt(gdt_ptr gdtr) { asm volatile("lgdt %0" :: "m"(gdtr)); }
	virtual void load_idt(uintptr_t idtr) { return 0; }
	virtual void load_tss(uint16_t idx)  { asm volatile("ltr %0" :: "r"(idx)); }

	virtual void disable_interrupts() { asm volatile("cli"); }
	virtual void enable_interrupts() { asm volatile("sti"); }

	virtual uintptr_t get_gdt() { return 0; }

//	virtual uint64_t read_reg64(char* reg) { uint64_t ret; READ_REG(reg); return ret; }
//	virtual uint32_t read_reg32(char* reg) { uint32_t ret; READ_REG(reg); return ret; }
//	virtual uint16_t read_reg16(char* reg) { uint16_t ret; READ_REG(reg); return ret; }
//	virtual uint8_t read_reg8(char* reg) { uint8_t ret; READ_REG(reg); return ret; }
//
//	virtual void write_reg64(char* reg, uint64_t val) { WRITE_REG(reg); }
//	virtual void write_reg32(char* reg, uint32_t val) { WRITE_REG(reg); }
//	virtual void write_reg16(char* reg, uint16_t val) { WRITE_REG(reg); }
//	virtual void write_reg8(char* reg, uint8_t val) { WRITE_REG(reg); }

};
