#pragma once
#include<cstdint>
#include<kernel/ptr.hpp>

#define TO_VIRT_ADDR(a) ((uintptr_t)a | Memory::hhdm_offset)
#define TO_PHYS_ADDR(a) ((uintptr_t)a ^ Memory::hhdm_offset)
#define PAGE_SIZE_BYTES 4096

#define SLAB_PTR_SIZE sizeof(slab_t)
#define SLAB_START_ADDRESS(s) &s + SLAB_PTR_SIZE
#define SLAB_END_ADDRESS(s) SLAB_START_ADDRESS(s) + s->size

extern uint64_t kernelend;

#define IS_ALIGNED(a) (a & 0xFFF) == 0

typedef struct frame_t;

typedef struct pt_entry {
	uintptr_t present : 1;
	uintptr_t rw : 1;
	uintptr_t user : 1;
	uintptr_t write_through : 1;
	uintptr_t cache_disabled : 1;
	uintptr_t accessed : 1;
	uintptr_t dirty : 1;
	uintptr_t pat : 1;
	uintptr_t global : 1;
	uintptr_t reserved : 2;
	uintptr_t phys_addr : 40;
	uintptr_t reserved2 : 1;
	uintptr_t ignored : 7;
	uintptr_t memory_protection_key : 4;
	uintptr_t no_exec : 1;
}__attribute__((packed)) pt_entry_t;

typedef struct pd_entry {
	uintptr_t present : 1;
	uintptr_t rw : 1;
	uintptr_t user : 1;
	uintptr_t write_through : 1;
	uintptr_t cache_disabled : 1;
	uintptr_t accessed : 1;
	uintptr_t ignored1 : 1;
	uintptr_t page_size : 1;
	uintptr_t ignored2 : 4;
	uintptr_t pt_address: 39;
	uintptr_t reserved : 1;
	uintptr_t ignored3 : 11;
	uintptr_t no_exec : 1;
}__attribute__((packed))  pd_entry_t;

typedef struct pdp_entry {
	uintptr_t present : 1;
	uintptr_t rw : 1;
	uintptr_t user : 1;
	uintptr_t write_through : 1;
	uintptr_t cache_disabled : 1;
	uintptr_t accessed : 1;
	uintptr_t ignored1 : 1;
	uintptr_t reserved1 : 1;
	uintptr_t ignored2 : 4;
	uintptr_t pd_address: 39;
	uintptr_t reserved2 : 1;
	uintptr_t ignored3 : 11;
	uintptr_t no_exec : 1;
} __attribute__((packed)) pdp_entry_t;

typedef struct pml4_entry {
	uintptr_t present : 1;
	uintptr_t rw : 1;
	uintptr_t user : 1;
	uintptr_t write_through : 1;
	uintptr_t cache_disabled : 1;
	uintptr_t accessed : 1;
	uintptr_t ignored1 : 1;
	uintptr_t page_size : 1;
	uintptr_t ignored2 : 4;
	uintptr_t pdp_address: 39;
	uintptr_t reserved : 1;
	uintptr_t ignored3 : 11;
	uintptr_t no_exec : 1;
} __attribute__((packed)) pml4_entry_t;

typedef struct pml4_dir {
	pml4_entry_t dir[512];
} __attribute__((aligned(PAGE_SIZE_BYTES))) pml4_dir_t;

typedef struct pdp_dir {
	pdp_entry_t dir[512];
} __attribute__((aligned(PAGE_SIZE_BYTES))) pdp_dir_t;

typedef struct pd_dir {
	pd_entry_t dir[512];
} __attribute__((aligned(PAGE_SIZE_BYTES))) pd_dir_t;

typedef struct pt_dir {
	pt_entry_t pages[512];
} __attribute__((aligned(PAGE_SIZE_BYTES))) pt_dir_t;
	
typedef struct page_map {
	pml4_dir* pml4 = nullptr;
	pdp_dir* pdp = nullptr;
	pd_dir* pd = nullptr;
	pt_dir* pt = nullptr;
} page_map_t;

struct frame_t {
	// Phys and virt addresses cached for convenience sake
	uintptr_t phys_addr;
	uintptr_t virt_addr;
	bool is_readonly;
	bool is_user;
	bool is_executable;
};

typedef struct slab_t;
struct slab_t {
//	uint64_t buffer = 0;
//	uint64_t buffer2 = 0;
//	uint64_t buffer3 = 0;
	slab_t* next;
	slab_t* previous;
	uint64_t size; //bytes
	bool is_free;
	bool is_readonly;
	bool is_executable;
};

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
	extern uint64_t total_mem_bytes;
	extern uint64_t usable_mem_bytes;
	extern uint64_t reserved_mem_bytes;
	extern uint64_t bad_mem_bytes;
	extern uint64_t reclaimable_mem_bytes;
	extern uint64_t acpi_nvs_mem_bytes;
	extern uint64_t unknown_mem_bytes;
	extern uint64_t virt_addr_offset;
	extern uint64_t hhdm_offset;
	extern uint64_t kernel_physical_addr_base;
	extern uint64_t kernel_virtual_addr_base;
	extern mem_region usable_memory_regions[7];
	namespace VMM {
		extern pml4_dir_t* pml4_dir;
		extern pdp_dir_t* cur_pdp_dir;
		extern pd_dir_t* cur_pd_dir;
		extern pt_dir_t* cur_pt_dir;
		void init();
		uintptr_t alloc_new_frame();
		uintptr_t alloc_new_table_or_next_entry(page_map_t table);
		void create_page_table_entry(frame_t* f, pml4_dir_t* pml4);
		void create_page_table_entry(frame_t* f, page_map_t map);
		void map_address(uintptr_t virt_addr, uintptr_t phys_addr, page_map_t map, bool readonly, bool executable, bool isuser);
		pt_entry_t* lookup_address(uintptr_t virt_addr);
	}
	namespace Allocation {
		void dump_slab_list();
		slab_t* get_slab_head();
		void kfree(void* vaddr);
		void ufree(void* vaddr);
		void* kallocate(uint64_t objsize, uint64_t num, bool align=false);
		void* uallocate(uint64_t objsize, uint64_t num, bool readonly, bool executable, bool align=false);
		void* kalloc(uint64_t objsize, uint64_t num);
		void* ualloc(uint64_t sizebytes, bool readonly, bool executable);

		void init();
		void test();
		void test_operators();
		void ptr_t_test();
	}
}


//God i hate defining functions inside headers
template<typename T>
ptr_t<T> kmalloc(uint64_t sizebytes) {
	void* ptr = Memory::Allocation::kallocate(sizebytes, 1);
	return ptr_t<T>((T)ptr, sizebytes);

}
