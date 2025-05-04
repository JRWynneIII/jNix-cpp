#pragma once
#include<cstdint>
#include<kernel/ptr.hpp>

#define TO_VIRT_ADDR(a) ((uintptr_t)a + Memory::hhdm_offset)
#define TO_PHYS_ADDR(a) ((uintptr_t)a - Memory::hhdm_offset)
#define PAGE_SIZE_BYTES 4096

#define SLAB_PTR_SIZE sizeof(slab_t)
#define SLAB_START_ADDRESS(s) &s + SLAB_PTR_SIZE
#define SLAB_END_ADDRESS(s) SLAB_START_ADDRESS(s) + s->size

//extern uint64_t kernelend;

#define IS_ALIGNED(a) (a & 0xFFF) == 0

typedef struct frame_t;

typedef union pte {
	struct {
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
	};
	uint64_t as_uint64_t;
}__attribute__((packed)) pt_entry_t;

typedef union pde {
	struct {
		uintptr_t present : 1;
		uintptr_t rw : 1;
		uintptr_t user : 1;
		uintptr_t write_through : 1;
		uintptr_t cache_disabled : 1;
		uintptr_t accessed : 1;
		uintptr_t ignored1 : 1;
		uintptr_t reserved0 : 1;
		uintptr_t ignored2 : 4;
		uintptr_t pt_address: 40;
		uintptr_t reserved : 1;
		uintptr_t ignored3 : 10;
		uintptr_t no_exec : 1;
	};
	uint64_t as_uint64_t;
}__attribute__((packed))  pd_entry_t;

typedef union pdpe {
	struct {
		uintptr_t present : 1;
		uintptr_t rw : 1;
		uintptr_t user : 1;
		uintptr_t write_through : 1;
		uintptr_t cache_disabled : 1;
		uintptr_t accessed : 1;
		uintptr_t ignored1 : 1;
		uintptr_t reserved1 : 1;
		uintptr_t ignored2 : 4;
		uintptr_t pd_address: 40;
		uintptr_t reserved2 : 1;
		uintptr_t ignored3 : 10;
		uintptr_t no_exec : 1;
	};
	uint64_t as_uint64_t;
} __attribute__((packed)) pdp_entry_t;

typedef union pml4e {
	struct {
		uintptr_t present : 1;
		uintptr_t rw : 1;
		uintptr_t user : 1;
		uintptr_t write_through : 1;
		uintptr_t cache_disabled : 1;
		uintptr_t accessed : 1;
		uintptr_t ignored1 : 1;
		uintptr_t reserved0 : 1;
		uintptr_t ignored2 : 4;
		uintptr_t pdp_address: 40;
		uintptr_t reserved : 1;
		uintptr_t ignored3 : 10;
		uintptr_t no_exec : 1;
	};
	uint64_t as_uint64_t;
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
	
//TODO: We probably should make this a list of each dir/table type
//	so that we can append the pointers for each new one we create
//	and so that we can free them up. 
typedef struct page_map {
	pml4_dir* pml4 = nullptr;
	pdp_dir* pdp = nullptr;
	pd_dir* pd = nullptr;
	pt_dir* pt = nullptr;
	uint64_t pml4_idx = 0;
	uint64_t pdp_idx = 0;
	uint64_t pd_idx = 0;
	uint64_t pt_idx = 0;

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

extern "C" uint8_t endkernel[];

class addrspace_t;

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
	extern mem_region kernel_region;
	extern mem_region bootloader_reclaimable_region;
	extern mem_region framebuffer_region;
	extern mem_region usable_memory_regions[7];
	extern mem_region unusable_memory_regions[7];
	namespace PMM {
		uint64_t num_frames_used();
		uintptr_t bitmap_location();
		uintptr_t alloc_frame();
		uintptr_t alloc_contiguous_frames(uint64_t num);
		void free_frame(uintptr_t frame);
		void free_contiguous_frames(uintptr_t frames, uint64_t num);
		void dump_pmm_info();
	}
	namespace VMM {
		extern page_map_t page_map_table;
		void create_page_table_entry(uintptr_t phys, uintptr_t virt, bool rw, bool noexec, bool isuser, addrspace_t* as);
		addrspace_t& kernel_address_space();
		void init();
		void create_page_table_entry(frame_t* frame, addrspace_t* as);
		void map_address(uintptr_t virt_addr, uintptr_t phys_addr, bool readonly, bool executable, bool isuser, addrspace_t* as);
		pt_entry_t* lookup_address(uintptr_t virt_addr);

		uintptr_t alloc_contiguous_pages(uint64_t num, bool rw, bool noexec, bool isuser, addrspace_t* as);
		uintptr_t alloc_contiguous_pages(uint64_t num, bool rw, bool noexec, bool isuser);

		void modify_page_perms(uintptr_t virt, bool rw, bool noexec, bool isuser, addrspace_t* as);
		void modify_page_perms(uintptr_t virt, bool rw, bool noexec, bool isuser);
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
