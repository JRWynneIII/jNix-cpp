#pragma once
#include<kernel/gdt.hpp>

class processor_t {
public:
	processor_t() {}
	~processor_t() {}
	virtual void load_gdt(gdt_ptr gdtr) = 0;
	virtual void load_idt(uintptr_t idtr) = 0;
	virtual void load_tss(uint16_t idx) = 0;
	virtual void disable_interrupts() = 0;
	virtual void enable_interrupts() = 0;
	virtual uintptr_t get_gdt() = 0;
//	virtual uint64_t read_reg64(char* reg) = 0;
//	virtual uint32_t read_reg32(char* reg) = 0;
//	virtual uint16_t read_reg16(char* reg) = 0;
//	virtual uint8_t read_reg8(char* reg) = 0;
//	virtual void write_reg64(char* reg, uint64_t val) = 0;
//	virtual void write_reg32(char* reg, uint32_t val) = 0;
//	virtual void write_reg16(char* reg, uint16_t val) = 0;
//	virtual void write_reg8(char* reg, uint8_t val) = 0;
};
