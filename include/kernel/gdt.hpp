#pragma once
#include<kernel.h>
#include<kernel/tss.hpp>

#define NUM_GDT_ENTRIES 7

typedef struct tss_gdt_entry {
	uint16_t limit;
	uint16_t base;
	uint16_t access;
	uint16_t flags;
} tss_gdt_entry_t;

//TODO: Change these to bitfield structs
typedef union gdt_entry_t {
	struct {
		uint16_t limit_lo 	: 16;
		uint32_t base_lo  	: 24;
		//access
		uint8_t accessed 	: 1;
		uint8_t rw 		: 1;
		uint8_t code_seg 	: 1; //conforming
		uint8_t executable	: 1; //code
		uint8_t code_or_data 	: 1; //code_data_segment
		uint8_t ring 		: 2; //DPL
		uint8_t present 	: 1;
		
		//granularity
		uint8_t limit_hi 	: 4;
		uint8_t available 	: 1;
		uint8_t long_mode 	: 1;
		uint8_t use_32bit_ops	: 1; //big
		uint8_t granularity 	: 1;

		uint8_t base_hi		: 8;
	}__attribute__((packed));
	uint64_t as_uint64_t;
	tss_gdt_entry_t as_tss_gdt_entry_t;
}__attribute__((packed)) gdt_entry_t;

typedef struct gdt_ptr {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed)) gdt_ptr;

extern "C" void reloadsegs();

class gdt_t {
private:
	gdt_entry_t gdt[NUM_GDT_ENTRIES];
	gdt_ptr gdtr;
	tss_t tss;
public:
	gdt_t();
	~gdt_t();
	void set_segment(int idx,
			uint64_t base,
			uint64_t limit,
			bool rw,
			bool is_code,
			bool code_or_data,
			uint8_t ring,
			bool granularity);

	void set_segment(int idx, uint64_t entry);
	void set_tss(int idx1, int idx2);
	void init();
	void old_init();
	void set_gate(int idx, uint64_t base, uint64_t limit, uint8_t access, uint8_t gran);
	void load_gdt();
	void load_tss();
	void dump_gdt();
};
