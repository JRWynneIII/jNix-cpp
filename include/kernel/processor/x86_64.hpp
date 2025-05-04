#pragma once
#include<kernel/gdt.hpp>
#include<kernel/tss.hpp>
#include<kernel/processor.hpp>

//We have to do this to basically 'template' the inline assembly. This sucks but eh
#define READ_REG(r) asm volatile("mov %%" #r ", %0" : "=r"(ret))
#define WRITE_REG(r,v) asm volatile("mov %0, %%rax;" "mov %%rax, %%" #r :: "g"(v) : "rax"); 

// TODO:
//To read/write registers, 
//	- create an enum of each register name
//	- add a switch/case for *each register* to the read_reg func where the reg name is hard coded

typedef enum x86_64_register {
	cr0,
	cr1,
	cr2,
	cr3,
	r15,
	r14,
	r13,
	r12,
	r11,
	r10,
	r9,
	r8,
	rbp,
	rdi,
	rsi,
	rdx,
	rcx,
	rbx,
	rax,
	eax,
	ax,
	rip,
	cs,
	rflags,
	rsp,
	ss
} x86_64_register;

class x86_64 : public processor_t {
public:
	x86_64() {}
	x86_64(processor_t& p) {}
	~x86_64() {}
	virtual void load_gdt(gdt_ptr gdtr) { asm volatile("lgdt %0" :: "m"(gdtr)); }
	virtual void load_idt(uintptr_t idtr) { return 0; }
	virtual void load_tss(uint16_t idx)  { asm volatile("ltr %0" :: "r"(idx)); }

	virtual void disable_interrupts() { asm volatile("cli"); }
	virtual void enable_interrupts() { asm volatile("sti"); }

	virtual uintptr_t get_gdt() { return 0; }

	virtual uint64_t read_reg(int reg) { 
		uint64_t ret; 
		switch(reg) {
			case x86_64_register::cr0:
				READ_REG(cr0);
				break;
			case x86_64_register::cr1:
				READ_REG(cr1);
				break;
			case x86_64_register::cr2:
				READ_REG(cr2);
				break;
			case x86_64_register::cr3:
				READ_REG(cr3);
				break;
			case x86_64_register::r15:
				READ_REG(r15);
				break;
			case x86_64_register::r14:
				READ_REG(r14);
				break;
			case x86_64_register::r13:
				READ_REG(r13);
				break;
			case x86_64_register::r12:
				READ_REG(r12);
				break;
			case x86_64_register::r11:
				READ_REG(r11);
				break;
			case x86_64_register::r10:
				READ_REG(r10);
				break;
			case x86_64_register::r9:
				READ_REG(r9);
				break;
			case x86_64_register::r8:
				READ_REG(r8);
				break;
			case x86_64_register::rbp:
				READ_REG(rbp);
				break;
			case x86_64_register::rdi:
				READ_REG(rdi);
				break;
			case x86_64_register::rsi:
				READ_REG(rsi);
				break;
			case x86_64_register::rdx:
				READ_REG(rdx);
				break;
			case x86_64_register::rcx:
				READ_REG(rcx);
				break;
			case x86_64_register::rbx:
				READ_REG(rbx);
				break;
			case x86_64_register::rax:
				READ_REG(rax);
				break;
		//	case x86_64_register::eax:
		//		READ_REG(eax);
		//		break;
		//	case x86_64_register::ax:
		//		READ_REG(ax);
		//		break;
			case x86_64_register::cs:
				READ_REG(cs);
				break;
			case x86_64_register::rsp:
				READ_REG(rsp);
				break;
			case x86_64_register::ss:
				READ_REG(ss);
				break;
			default:
				ret = 0;
		}
		return ret; 
	}

	virtual void write_reg(int reg, uint64_t val) { 
		switch(reg) {
			case x86_64_register::cr3:
				WRITE_REG(cr3, val);
				break;
			case x86_64_register::r15:
				WRITE_REG(r15, val);
				break;
			case x86_64_register::r14:
				WRITE_REG(r14, val);
				break;
			case x86_64_register::r13:
				WRITE_REG(r13, val);
				break;
			case x86_64_register::r12:
				WRITE_REG(r12, val);
				break;
			case x86_64_register::r11:
				WRITE_REG(r11, val);
				break;
			case x86_64_register::r10:
				WRITE_REG(r10, val);
				break;
			case x86_64_register::r9:
				WRITE_REG(r9, val);
				break;
			case x86_64_register::r8:
				WRITE_REG(r8, val);
				break;
			case x86_64_register::rbp:
				WRITE_REG(rbp, val);
				break;
			case x86_64_register::rdi:
				WRITE_REG(rdi, val);
				break;
			case x86_64_register::rsi:
				WRITE_REG(rsi, val);
				break;
			case x86_64_register::rdx:
				WRITE_REG(rdx, val);
				break;
			case x86_64_register::rcx:
				WRITE_REG(rcx, val);
				break;
			case x86_64_register::rbx:
				WRITE_REG(rbx, val);
				break;
			case x86_64_register::rax:
				WRITE_REG(rax, val);
				break;
		//	case x86_64_register::eax:
		//		WRITE_REG(eax, val);
		//		break;
		//	case x86_64_register::ax:
		//		WRITE_REG(ax, val);
		//		break;
			case x86_64_register::cs:
				WRITE_REG(cs, val);
				break;
			case x86_64_register::rsp:
				WRITE_REG(rsp, val);
				break;
			case x86_64_register::ss:
				WRITE_REG(ss, val);
				break;
		}
	}

};
