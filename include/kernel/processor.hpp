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
	virtual uint64_t read_reg(int reg) = 0;
	virtual void write_reg(int reg, uint64_t val) = 0;
};
