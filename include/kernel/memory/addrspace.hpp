#pragma once
#include<cstdint>
#include<cstddef>
#include<kernel/memory.h>

class addrspace_t {
private:
	bool is_kernel_addrspace;
	bool is_userspace;
	uintptr_t virt_addr_offset;
	uint64_t page_size_bytes;
	uintptr_t last_cr3_value;

	uintptr_t read_cr3();
	void write_cr3(uintptr_t val);

	void map_region(mem_region region, bool rw, bool isuserspace, bool notexecutable, uintptr_t virt_offset=0);

	void map_accessible_space();
	void map_kernel_region(mem_region kernel);

	uintptr_t alloc_zeroed_frame();
	void delete_table(uintptr_t address);

public:
	pml4_dir_t* pml4;

	addrspace_t(uintptr_t offset, bool is_user);
	//Copy constructor
	addrspace_t(const addrspace_t& rhs);

	~addrspace_t();

	void bootstrap();
	void init();
	void activate();
	void deactivate();

	void map_page(pml4_dir_t* pml4, 
		uintptr_t virt_addr, 
		uintptr_t phys_addr, 
		bool rw, 
		bool notexecutable, 
		bool writethrough, 
		bool disablecache, 
		bool isuserspace
	);

	void map_page(uintptr_t virt_addr, 
		uintptr_t phys_addr, 
		bool rw, 
		bool notexecutable, 
		bool writethrough, 
		bool disablecache, 
		bool isuserspace
		);

	void unmap(uintptr_t virt_addr);
	pt_entry_t* lookup(uintptr_t virt_addr);
	uintptr_t phys_to_virt(uintptr_t v) { return (v | this->virt_addr_offset); }
	uintptr_t virt_to_phys(uintptr_t p) { return (p ^ this->virt_addr_offset); }
};
