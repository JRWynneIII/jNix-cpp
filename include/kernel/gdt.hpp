#pragma once
#include<kernel.h>
#include<kernel/tss.hpp>

#define NUM_GDT_ENTRIES 7
#define GDT_NULL_IDX 0
#define GDT_KERNEL_CODE_IDX 1
#define GDT_KERNEL_DATA_IDX 2
#define GDT_USER_CODE_IDX 3
#define GDT_USER_DATA_IDX 4
#define GDT_TSS_IDX 5
#define GDT_TSS_IDX2 6

#define GDT_KERNEL_CODE_OFFSET GDT_KERNEL_CODE_IDX * 8
#define GDT_KERNEL_DATA_OFFSET GDT_KERNEL_DATA_IDX * 8
#define GDT_USER_CODE_OFFSET GDT_USER_CODE_IDX * 8
#define GDT_USER_DATA_OFFSET GDT_USER_DATA_IDX * 8

typedef struct tss_gdt_desc_lo {
	uint64_t limit_lo	:16;
	uint64_t base_lo	:24;

	//uint8_t base_mid_lo	:8;
	//access
	uint64_t type		:5;
	//uint8_t reserved0	:1;
	uint64_t dpl		:2;
	uint64_t present	:1;

	//limit
	uint64_t limit_hi	:4;

	//flags
	uint64_t available	:1;
	uint64_t reserved1	:2;
	uint64_t granularity	:1;

	//base
	uint64_t base_mid	:8;
}__attribute__((packed)) tss_gdt_entry_lo_t;

typedef struct tss_gdt_desc_hi {
	uint64_t base_hi	:32;

	uint64_t reserved2	:32;
}__attribute__((packed)) tss_gdt_entry_hi_t;

//typedef struct tss_gdt_entry {
//	uint16_t limit;
//	uint16_t base;
//	uint16_t access;
//	uint16_t flags;
//} tss_gdt_entry_t;

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
	tss_gdt_entry_lo_t as_tss_gdt_entry_lo_t;
	tss_gdt_entry_hi_t as_tss_gdt_entry_hi_t;

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
public:
	tss_t tss;
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
