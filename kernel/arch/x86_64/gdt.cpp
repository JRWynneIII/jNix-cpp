#include<cstdint>
#include<cstddef>
#include<kernel.h>
#include<kernel/gdt.hpp>
#include<kernel/tss.hpp>
#include<string.h>
#include<kernel/hardware.hpp>

gdt_t::gdt_t() {}

gdt_t::~gdt_t() {}

void gdt_t::dump_gdt() {
	for (int i = 0; i < NUM_GDT_ENTRIES; i++) 
		printfk("GDT Entry #%d: %x\n", i, this->gdt[i].as_uint64_t);
}

void gdt_t::set_segment(int idx, uint64_t entry) {
	this->gdt[idx].as_uint64_t = entry;
}

void gdt_t::set_segment(int idx,
			uint64_t base,
			uint64_t limit,
			bool rw,
			bool is_code,
			bool code_or_data,
			uint8_t ring,
			bool granularity) {
	this->gdt[idx] = {
		.limit_lo = limit & 0xFFFF,
		.base_lo  = (base >> 16),
		//access
		.accessed = 1,
		.rw = rw,
		.code_seg = is_code ? 1 : 0,
		.executable = is_code ? 1 : 0,
		.code_or_data = code_or_data ? 1 : 0,
		.ring = ring,
		.present = 1,
		//granularity
		.limit_hi = (limit & (0xf << 16)) >> 16, 
		.available = 1,
		.long_mode = 1,
		.use_32bit_ops = 0,
		.granularity = granularity ? 1 : 0,

		.base_hi = (base >> 24) & 0xFF
	};
}

extern "C" uint64_t stack_top;

void gdt_t::set_tss(int idx1, int idx2) {
	uintptr_t base = (uintptr_t)&(this->tss);
	uintptr_t limit = sizeof(tss_t) - 1;

	// In long mode, we actually need 2 TSS entries, one for ring 3, and one for ring 0, respectively
	this->gdt[idx1].as_tss_gdt_entry_t = {
		.limit = (uint16_t)limit,
		.base = (uint16_t)base,
		.access = (uint16_t)(0xE900 + ((base >> 16) & 0xFF)),
		.flags = (uint16_t)((base >> 16) & 0xFF00)
	};

	this->gdt[idx2].as_tss_gdt_entry_t = {
		.limit = (uint16_t) (limit >> 32),
		.base = (uint16_t) (base >> 48),
		.access = 0x0,
		.flags = 0x0
	};

	memset(&(this->tss), 0, sizeof(tss_t));
	//TODO: UNSURE
	this->tss.iopb = 0xdfff;
//	this->tss.rsp0 = &stack_top;
}

void gdt_t::load_tss() {
	logfk(KERNEL, "Loading TSS\n");
	//0x28 is the offset into the GDT for the first TSS descriptor
	Hardware::CPU().load_tss(0x28);
}

void gdt_t::load_gdt() {
	logfk(KERNEL, "Loading GDT\n");
	Hardware::CPU().load_gdt(this->gdtr);
}

void gdt_t::init() {
	Hardware::CPU().disable_interrupts();
	this->gdtr.limit = (sizeof(gdt_entry_t) * NUM_GDT_ENTRIES) - 1;
	this->gdtr.base = (uint64_t)&(this->gdt);
	// NULL
	this->set_segment(0, 0);
	// Kernel Code 64
	this->set_segment(1, 0, 0xFFFFFFFF, false, true, true, 0, true);
	// Kernel Data 64
	this->set_segment(2, 0, 0xFFFFFFFF, true, false, true, 0, true);
	// User Code 64
	this->set_segment(3, 0, 0xFFFFFFFF, false, true, true, 3, true);
	// User Data 64
	this->set_segment(4, 0, 0xFFFFFFFF, true, false, true, 3, true);
	
	//TSS
	this->set_tss(5, 6);
	this->load_gdt();

	reloadsegs();

	this->load_tss();

	Hardware::CPU().enable_interrupts();
}

namespace GDT {
	gdt_t& gdt() {
		static gdt_t g = gdt_t();
		return g;
	}

	void init() {
		gdt().init();
	}

	void dump() {
		gdt().dump_gdt();
	}
}
