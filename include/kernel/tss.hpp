#pragma once

typedef struct tss_entry {
	uint32_t reserved0	:32;

	uint64_t rsp0		:64;
	uint64_t rsp1		:64;
	uint64_t rsp2		:64;

	uint32_t reserved1	:32;
	uint32_t reserved2	:32;
	
	uint64_t ist1		:64;
	uint64_t ist2		:64;
	uint64_t ist3		:64;
	uint64_t ist4		:64;
	uint64_t ist5		:64;
	uint64_t ist6		:64;
	uint64_t ist7		:64;
	
	uint32_t reserved3	:32;
	uint32_t reserved4	:32;
	uint16_t reserved5	:16;
	uint16_t iopb		:16;
}__attribute__((packed, aligned(128))) tss_t;
