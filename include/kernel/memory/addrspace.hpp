#pragma once
#include<cstdint>
#include<cstddef>
#include<kernel/memory.h>

//class as_node {
//	uintptr_t address;
//	as_node_t* next;
//	as_node_t(uintptr_t a) : address(a), next(nullptr) {}
//};
//
//// A very simple vector that keeps track of the frames that it allocates for itself, and frees them 
//// upon destruction. This is necessary because we don't have malloc/a virtual memory allocator
//class as_vector {
//	as_node_t* data_head;
//	as_node_t* frames;
//	uint64_t size_bytes;
//	uint64_t length;
//	uint64_t nodes_per_frame;
//
//	as_vector() {
//		this->data_head = nullptr;
//		uintptr_t frame = Memory::PMM::alloc_frame();
//		this->frames = as_node(frame);
//		this->nodes_per_frame = 4096 / sizeof(as_node);
//	}
//
//	~as_vector() {
//		as_node_t* cur = this->frames;
//		while (cur != nullptr) {
//			Memory::PMM::free_frame(cur->address);
//			cur = cur->next;
//		}
//	}
//
//	void push_back(uintptr_t addr) {
//		as_node_t* cur = this->data_head;
//		while (cur->next != nullptr) {
//			cur = cur->next;
//		}
//
//		uintptr_t node_addr = // use nodes_per_frame to determine which entry in `frames` we need to start as our `base`
//				      // base_frame.address + (sizeof(as_node) * num_nodes_in_base_frame)
//				      // If node_addr >= base_frame.address + 4096, then allocate new frame and recalculate
//				      // based upon new address.
//		cur->next = as_node_t(addr);
//
//
//		//Check frames vs number of elements, see if we have enough space
//		//if not allocate new frame and push back at end of frames
//		//push addr to the end of data_head
//	}
//
//	uintptr_t pop_head() {
//	}
//};


class addrspace_t {
private:
	bool is_kernel_addrspace;
	bool is_userspace;
	uintptr_t virt_addr_offset;
	uint64_t page_size_bytes;

	uintptr_t read_cr3();
	void write_cr3(uintptr_t val);

	void map_region(mem_region region, bool rw, bool isuserspace, bool notexecutable, uintptr_t virt_offset=0);

	void map_accessible_space();
	void map_kernel_region(mem_region kernel);

	uintptr_t alloc_zeroed_frame();
	void delete_table(uintptr_t address);

public:
	pml4_dir_t* pml4;

	addrspace_t(uintptr_t offset);
	//Copy constructor
	addrspace_t(const addrspace_t& rhs);

	~addrspace_t();

	void bootstrap(pml4_dir_t* old_pml4);
	void init();

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
