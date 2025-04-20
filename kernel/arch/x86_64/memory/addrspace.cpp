#include<cstddef>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<kernel.h>
#include<kernel/memory.h>
#include<kernel/panic.h>
#include<kernel/memory/addrspace.hpp>
#include<kernel/ptr.hpp>
#include<cstdint>

addrspace_t::addrspace_t(uintptr_t offset, bool is_user) : pml4(nullptr),
					is_userspace(is_user), 
					is_kernel_addrspace(!is_user),
					virt_addr_offset(offset), 
					page_size_bytes(4096) {}

//Copy Constructor
addrspace_t::addrspace_t(const addrspace_t& rhs) {
	//TODO: memcpy all entries of pml4, pdp, pd, pt from rhs. Will require walking the whole table
}

addrspace_t::~addrspace_t() {
	// Traverse through the page map and de-allocate all frames 
	for (int pml4_idx = 0; pml4_idx < 512 ; pml4_idx++) {
		pml4_entry_t pml4e = this->pml4->dir[pml4_idx];
		if (pml4e.pdp_address != 0x0) {
			for (int pdp_idx = 0; pdp_idx < 512; pdp_idx++) {
				pdp_entry_t pdpe = ((pdp_dir_t*)pml4e.pdp_address)->dir[pdp_idx];
				if (pdpe.pd_address != 0x0) {
					for (int pd_idx = 0; pd_idx < 512; pd_idx++) {
						pd_entry_t pde = ((pd_dir_t*)pdpe.pd_address)->dir[pd_idx];
						if (pde.pt_address != 0) {
								this->delete_table(pde.pt_address);
						}
					}
					this->delete_table(pdpe.pd_address);
				}
			}
			this->delete_table(pml4e.pdp_address);
		}
	}
	this->delete_table(this->pml4);
}

void addrspace_t::delete_table(uintptr_t address) {
	if (this->is_kernel_addrspace) {
		//We manually free the frame here since we manually allocated it. Maybe 0 it out?
		Memory::PMM::free_frame(address);
	} else {
		delete reinterpret_cast<void*>(address);
	}
}

uintptr_t addrspace_t::read_cr3() {
	uintptr_t pml4_ptr;
	asm volatile("mov %%cr3, %0" : "=r"(pml4_ptr));
	return (pml4_ptr & ~0xFFF);
}

void addrspace_t::write_cr3(uint64_t ptr) {
	//ptr &= ~0xFFF;
	asm volatile("mov %0, %%rax;" 
			"mov %%rax, %%cr3" 
			: 
			: "g"(ptr) 
			: "rax");
}

void addrspace_t::activate() {
	this->last_cr3_value = this->read_cr3();
	this->write_cr3(this->virt_to_phys(this->pml4->dir));
}

void addrspace_t::deactivate() {
	this->write_cr3(this->last_cr3_value);
}

//	- Implement init(). this will:
//		- Call `new` or `kallocate` to create new page map tables.
//			- this will automatically add them to the kernel's addrspace_t
//		- Zero out these tables
//		- Add self referencial entries
//			- or recursive entries (optional, see above?)
//		- Does NOT set cr3 or copies anything existing. These will be done by separate methods to come later
//

uintptr_t addrspace_t::alloc_zeroed_frame() {
	uintptr_t frame = Memory::PMM::alloc_frame();
	memset(this->phys_to_virt(frame), 0, 4096);
	return frame;
}

void addrspace_t::bootstrap() {
	uintptr_t new_tablespace = this->alloc_zeroed_frame();
	uintptr_t new_pml4_location = new_tablespace;

	uintptr_t new_virt_pml4_location = this->phys_to_virt(new_pml4_location);
	this->pml4 = (pml4_dir_t*)new_virt_pml4_location;
	logfk(KERNEL, "VMM: Initializing new PML4: %x\n", this->pml4);

	logfk(KERNEL, "VMM: Mapping all accessible space\n");
	this->map_accessible_space();
	logfk(KERNEL, "VMM: Mapping kernel pages\n");
	this->map_kernel_region(Memory::kernel_region);
	logfk(KERNEL, "VMM: Mapping framebuffer pages\n");
	this->map_region(Memory::framebuffer_region, true, false, false);

	// Map pages for the PMM bitmap
	uintptr_t pmm_bitmap_location = Memory::PMM::bitmap_location();
	uint64_t pmm_bitmap_num_frames = Memory::PMM::num_frames_used();

	mem_region pmm_region = {
		.base = pmm_bitmap_location,
		.length = pmm_bitmap_num_frames * this->page_size_bytes
	};

	logfk(KERNEL, "VMM: Mapping PMM bitmap location\n");
	this->map_region(pmm_region, true, false, false);
}

void addrspace_t::map_accessible_space() {
	//Get the highest address of accessible data
	uintptr_t last_usable_address = 0;
	uintptr_t first_usable_address = 0;
	for (auto r : Memory::usable_memory_regions) {
		uintptr_t end_of_region = r.base + r.length;
		last_usable_address = ((end_of_region > last_usable_address) ? end_of_region : last_usable_address);
	}
	//Test kernel region
	uintptr_t end_of_region = Memory::kernel_region.base + Memory::kernel_region.length;
	last_usable_address = ((end_of_region > last_usable_address) ? end_of_region : last_usable_address);

	//Test bootloader reclaimable region
	end_of_region = Memory::bootloader_reclaimable_region.base + Memory::bootloader_reclaimable_region.length;
	last_usable_address = ((end_of_region > last_usable_address) ? end_of_region : last_usable_address);

	last_usable_address &= ~(0xFFF);
//	logfk(KERNEL, "VMM: Mapping pages from %x (%x) to %x (%x)\n", 
//			first_usable_address, 
//			this->phys_to_virt(first_usable_address),
//			last_usable_address,
//			this->phys_to_virt(last_usable_address)
//	);

	uintptr_t cur = first_usable_address;
	while (cur <= last_usable_address) {
		uintptr_t virt_cur = this->phys_to_virt(cur);
		this->map_page(virt_cur, cur, true, false, false, false, false);
		cur += this->page_size_bytes;
	}
}

void addrspace_t::map_kernel_region(mem_region region) {
	uintptr_t region_start = region.base;
	uintptr_t region_end = region.base + region.length;
	uintptr_t region_start_virt = Memory::kernel_virtual_addr_base + region_start - Memory::kernel_physical_addr_base;
	uintptr_t region_end_virt = Memory::kernel_virtual_addr_base + region_end - Memory::kernel_physical_addr_base;

	uintptr_t cur = region_start;
	while (cur < region_end) {
		uintptr_t cur_virt = Memory::kernel_virtual_addr_base + cur - Memory::kernel_physical_addr_base;
		this->map_page(cur_virt, cur, true, true, false, false, false);
		cur = cur + 4096;
	}
}

void addrspace_t::map_region(mem_region region, bool rw, bool isuserspace, bool notexecutable, uintptr_t virt_offset=0) {
	if (virt_offset == 0) {
		virt_offset = this->virt_addr_offset;
	}

	uintptr_t region_start = region.base;
	uintptr_t region_end = region.base + region.length;

	//Make sure we're 4k aligned
	region_start &= ~0xFFF;

	uintptr_t cur = region_start;
	while (cur < region_end) {
		uintptr_t cur_virt = cur + virt_offset;
		this->map_page(cur_virt, cur, rw, notexecutable, false, false, isuserspace);
		cur += this->page_size_bytes;
	}
}

void addrspace_t::map_page(pml4_dir_t* pml4, 
		uintptr_t virt_addr, 
		uintptr_t phys_addr, 
		bool rw, 
		bool notexecutable, 
		bool writethrough, 
		bool disablecache, 
		bool isuserspace) {
	//find offsets into each table
	uint64_t pml4_offset = ((virt_addr >> 12) >> 27) & 0x1FF;
	uint64_t pdp_offset = ((virt_addr >> 12) >> 18) & 0x1FF;
	uint64_t pd_offset = ((virt_addr >> 12) >> 9) & 0x1FF;
	uint64_t pt_offset = (virt_addr >> 12) & 0x1FF;

	//Ensure its 4k aligned
	phys_addr &= ~0xFFF;

	pml4_entry_t pml4e = pml4->dir[pml4_offset];
	if (!pml4e.present) {
		//Write new entry
		uintptr_t pdp_table = this->alloc_zeroed_frame();
		pml4_entry_t new_entry = {
			.present = 1,
			.rw = rw,
			.user = 1,
			.pdp_address = pdp_table >> 12,
		};

		pml4->dir[pml4_offset] = new_entry;
		pml4e = new_entry;
	}

	pdp_dir_t* pdp_dir = (pdp_dir_t*)(this->phys_to_virt(pml4e.pdp_address << 12));
	pdp_entry_t pdpe = pdp_dir->dir[pdp_offset];
	if (!pdpe.present) {
		//Write new entry
		uintptr_t pd_table = this->alloc_zeroed_frame();
		pdp_entry_t new_entry = {
			.present = 1,
			.rw = rw,
			.user = 1,
			.pd_address = pd_table >> 12,
		};

		pdp_dir->dir[pdp_offset] = new_entry;
		pdpe = new_entry;
	}

	pd_dir_t* pd_dir = (pd_dir_t*)(this->phys_to_virt(pdpe.pd_address << 12));
	pd_entry_t pde = pd_dir->dir[pd_offset];
	if (!pde.present) {
		//Write new entry
		uintptr_t pt_table = this->alloc_zeroed_frame();
		pd_entry_t new_entry = {
			.present = 1,
			.rw = rw,
			.user = 1,
			.pt_address = pt_table >> 12,
		};

		pd_dir->dir[pd_offset] = new_entry;
		pde = new_entry;
	}

	pt_dir_t* pt_dir = (pt_dir_t*)(this->phys_to_virt(pde.pt_address << 12));

	pt_entry_t new_entry = {
		.present = 1,
		.rw = rw,
		.user = isuserspace,
		.write_through = writethrough,
		.cache_disabled = disablecache,
	};

	new_entry.as_uint64_t = ((phys_addr) & ~0xFFF) | new_entry.as_uint64_t;

	pt_dir->pages[pt_offset] = new_entry;
}

void addrspace_t::map_page(uintptr_t virt_addr, 
		uintptr_t phys_addr, 
		bool rw, 
		bool notexecutable, 
		bool writethrough, 
		bool disablecache, 
		bool isuserspace) {
	this->map_page(this->pml4, virt_addr, phys_addr, rw, notexecutable, writethrough, disablecache, isuserspace);
}

void addrspace_t::unmap(uintptr_t virt_addr) {
	pt_entry_t* pte = this->lookup(virt_addr);
	pte->present = false;
	//TODO: invalidate page
}

pt_entry_t* addrspace_t::lookup(uintptr_t virt_addr) {
	//find offsets into each table
	uint64_t pml4_offset = ((virt_addr >> 12) >> 27) & 0x1FF;
	uint64_t pdp_offset = ((virt_addr >> 12) >> 18) & 0x1FF;
	uint64_t pd_offset = ((virt_addr >> 12) >> 9) & 0x1FF;
	uint64_t pt_offset = (virt_addr >> 12) & 0x1FF;
	uint64_t phys_offset = virt_addr & 0xFFF;

	pml4_entry_t pml4e = pml4->dir[pml4_offset];
	if (!pml4e.present) return nullptr;

	pdp_dir_t* pdp_dir = (pdp_dir_t*)(this->phys_to_virt(pml4e.pdp_address << 12));
	pdp_entry_t pdpe = pdp_dir->dir[pdp_offset];
	if (!pdpe.present) return nullptr;

	pd_dir_t* pd_dir = (pd_dir_t*)(this->phys_to_virt(pdpe.pd_address << 12));
	pd_entry_t pde = pd_dir->dir[pd_offset];
	if (!pde.present) return nullptr;

	pt_dir_t* pt_dir = (pt_dir_t*)(this->phys_to_virt(pde.pt_address << 12));

	return (pt_entry_t*)((uintptr_t)pt_dir->pages + pt_offset);
}
