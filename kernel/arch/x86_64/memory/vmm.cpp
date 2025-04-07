#include<cstddef>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<kernel.h>
#include<kernel/memory.h>
#include<kernel/ptr.hpp>
#include<cstdint>

namespace Memory {
	namespace VMM {
		pml4_dir_t* pml4_dir;
		pdp_dir_t*  cur_pdp_dir;
		pd_dir_t*   cur_pd_dir;
		pt_dir_t*   cur_pt_dir;

		uintptr_t alloc_new_frame() {
			uintptr_t base = Memory::usable_memory_regions[0].base;
			// Don't override last_dir_alloc if its already set
			static uintptr_t end_dir_addr = end_dir_addr ? end_dir_addr : base;

			void* old_base = end_dir_addr;
			//Shouldn't we 0 out the table? I feel like we should....
			//memset(old_base, 0, 4096);
			end_dir_addr += 4096;
			return old_base; 
		}

		uintptr_t alloc_new_table_or_next_entry(page_map_t table) {
			static uint64_t pml4_idx = 0;
			static uint64_t pdp_idx = 0;
			static uint64_t pd_idx = 0;
			static uint64_t pt_idx = 0;

			if (table.pml4 != nullptr) {
				pml4_idx++;
				return &(pml4_dir->dir[pml4_idx]);
			} else if (table.pdp != nullptr) {
				if (pdp_idx == 512) {
					uintptr_t new_frame = alloc_new_frame();
					pdp_idx = 0;
					cur_pdp_dir = new_frame;
				}
				return &(cur_pdp_dir->dir[pdp_idx]);
			} else if (table.pd != nullptr) {
				if (pd_idx == 512) {
					uintptr_t new_frame = alloc_new_frame();
					pd_idx = 0;
					cur_pd_dir = new_frame;
				}
				return &(cur_pd_dir->dir[pd_idx]);
			} else if (table.pt != nullptr) {
				if (pt_idx == 512) {
					uintptr_t new_frame = alloc_new_frame();
					pt_idx = 0;
					cur_pt_dir = new_frame;
				}
				return &(cur_pt_dir->pages[pt_idx]);
			}
			return reinterpret_cast<uintptr_t>(nullptr);
		}
		
		pml4_entry_t* find_or_create_pml4_entry(pml4_entry_t* address, uint64_t offset, page_map_t* map = nullptr) {
			pml4_entry_t* entry = (pml4_entry_t*)(address + offset);
			if (!entry->present) {
				page_map_t table;
				if (map != nullptr) table = *map;
				else table.pdp = cur_pdp_dir;

				uintptr_t pdp_addr = alloc_new_table_or_next_entry(table);
				//uintptr_t pdp_addr = alloc_dir_entry();
				// Set to all 0's
				memset(TO_VIRT_ADDR(pdp_addr), 0, sizeof(pdp_entry_t));
				//create entry
				pml4_entry_t new_entry;
				new_entry.present = 1;
				new_entry.rw = 1;
				new_entry.pdp_address = pdp_addr;
				// Write the pml4 entry
				memcpy(entry, &new_entry, sizeof(pml4_entry_t));

			} else {
				return entry;
			}
		}

		pdp_entry_t* find_or_create_pdp_entry(uintptr_t address, uint64_t offset, page_map_t* map = nullptr) {
			address = TO_VIRT_ADDR(address);
			pdp_entry_t* entry = ((pdp_dir_t*)address)->dir + offset;
			if (!entry->present) {
				page_map_t table;
				if (map != nullptr) table = *map;
				else table.pd = cur_pd_dir;

				uintptr_t pd_addr = alloc_new_table_or_next_entry(table);
				//uintptr_t pd_addr = alloc_dir_entry();
				// Set to all 0's
				memset(TO_VIRT_ADDR(pd_addr), 0, sizeof(pd_entry_t));
				//create entry
				pdp_entry_t new_entry;
				new_entry.present = 1;
				new_entry.rw = 1;
				new_entry.pd_address = pd_addr;
				// Write the pdp entry
				memcpy(entry, &new_entry, sizeof(pdp_entry_t));

			} else {
				return entry;
			}
		}

		pd_entry_t* find_or_create_pd_entry(uintptr_t address, uint64_t offset, page_map_t* map = nullptr) {
			address = TO_VIRT_ADDR(address);
			pd_entry_t* entry = ((pd_dir_t*)address)->dir + offset;
			if (!entry->present) {
				page_map_t table;
				if (map != nullptr) table = *map;
				else table.pt = cur_pt_dir;

				uintptr_t pt_addr = alloc_new_table_or_next_entry(table);
				//uintptr_t pt_addr = alloc_dir_entry();
				// Set to all 0's
				memset(TO_VIRT_ADDR(pt_addr), 0, sizeof(pt_entry_t));
				//create entry
				pd_entry_t new_entry;
				new_entry.present = 1;
				new_entry.rw = 1;
				new_entry.pt_address = pt_addr;
				// Write the pd entry
				memcpy(entry, &new_entry, sizeof(pd_entry_t));

			} else {
				return entry;
			}
		}

		pt_entry_t* find_or_create_pt_entry(uintptr_t address, uint64_t offset, uintptr_t phys_addr, bool readonly, bool isuser, bool executable) {
			address = TO_VIRT_ADDR(address);
			pt_entry_t* entry = ((pt_dir_t*)address)->pages + offset;
			if (!entry->present) {
				//create entry
				pt_entry_t new_entry;
				new_entry.present = 1;
				new_entry.rw = 1;
				new_entry.phys_addr = phys_addr;
				if (isuser) new_entry.user = 1;
				if (executable) new_entry.no_exec = 0;
				if (readonly){
					new_entry.rw = 0;
					new_entry.no_exec = 1;
				}
				// Write the pt entry
				memcpy(entry, &new_entry, sizeof(pt_entry_t));

			} else {
				return entry;
			}
		}


		//flist can be one or an array of frames
		void create_page_table_entry(frame_t* f, pml4_dir_t* pml4) {
			//find offsets into each table
			// all offsets can be calculated from the vaddr
			uint64_t pml4_offset = ((f->virt_addr >> 12) >> 27) & 0x1FF;
			uint64_t pdp_offset = ((f->virt_addr >> 12) >> 18) & 0x1FF;
			uint64_t pd_offset = ((f->virt_addr >> 12) >> 9) & 0x1FF;
			uint64_t pt_offset = (f->virt_addr >> 12) & 0x1FF;

			uint64_t phys_offset = f->virt_addr & 0xFFF;

			pml4_entry_t* pml = find_or_create_pml4_entry(pml4->dir, pml4_offset);
			pdp_entry_t* pdp = find_or_create_pdp_entry(pml->pdp_address, pdp_offset);
			pd_entry_t* pd = find_or_create_pd_entry(pdp->pd_address, pd_offset);
			pt_entry_t* pt = find_or_create_pt_entry(pd->pt_address, pt_offset, f->phys_addr, f->is_readonly, f->is_user, f->is_executable);
		}

		void create_page_table_entry(frame_t* f, page_map_t map) {
			//find offsets into each table
			// all offsets can be calculated from the vaddr
			uint64_t pml4_offset = ((f->virt_addr >> 12) >> 27) & 0x1FF;
			uint64_t pdp_offset = ((f->virt_addr >> 12) >> 18) & 0x1FF;
			uint64_t pd_offset = ((f->virt_addr >> 12) >> 9) & 0x1FF;
			uint64_t pt_offset = (f->virt_addr >> 12) & 0x1FF;

			uint64_t phys_offset = f->virt_addr & 0xFFF;

			pml4_entry_t* pml = find_or_create_pml4_entry(map.pml4->dir, pml4_offset, &map);
			pdp_entry_t* pdp = find_or_create_pdp_entry(pml->pdp_address, pdp_offset, &map);
			pd_entry_t* pd = find_or_create_pd_entry(pdp->pd_address, pd_offset, &map);
			pt_entry_t* pt = find_or_create_pt_entry(pd->pt_address, pt_offset, f->phys_addr, f->is_readonly, f->is_user, f->is_executable);
		}

		//Convenience wrapper for create_page_table_entry so that you don't have to build your own frame object
		void map_address(uintptr_t virt_addr, uintptr_t phys_addr, page_map_t map, bool readonly, bool executable, bool isuser) {
			frame_t f = {
				.phys_addr = phys_addr,
				.virt_addr = virt_addr,
				.is_readonly = readonly,
				.is_user = isuser,
				.is_executable = executable,
			};
			create_page_table_entry(&f, map.pml4);
		}

		pt_entry_t* lookup_address(uintptr_t virt_addr) {
			uint64_t pml4_offset = ((virt_addr >> 12) >> 27) & 0x1FF;
			uint64_t pdp_offset = ((virt_addr >> 12) >> 18) & 0x1FF;
			uint64_t pd_offset = ((virt_addr >> 12) >> 9) & 0x1FF;
			uint64_t pt_offset = (virt_addr >> 12) & 0x1FF;

			uint64_t phys_offset = virt_addr & 0xFFF;

			pdp_dir_t* pdp_dir = pml4_dir->dir[pml4_offset].pdp_address;
			if (pdp_dir == 0x0) {
				return nullptr;
			} else {
				pdp_dir = TO_VIRT_ADDR(pdp_dir);
			}
			pd_dir_t* pd_dir = pdp_dir->dir[pdp_offset].pd_address;
			if (pd_dir == 0x0) {
				return nullptr;
			} else {
				pd_dir = TO_VIRT_ADDR(pd_dir);
			}
			pt_dir_t* pt_table = pd_dir->dir[pd_offset].pt_address;
			if (pt_table == 0x0) {
				return nullptr;
			} else {
				pt_table = TO_VIRT_ADDR(pt_table);
			}
			return TO_VIRT_ADDR(&pt_table->pages[pt_offset]);
		}

		void init() {
			// The page directory address is stored in CR3; This is a unique register per *thread*
			// NOTE: the above when implementing SMT
			uintptr_t pml4_ptr;
			asm volatile("mov %%cr3, %0" : "=r"(pml4_ptr));
			// CR3 holds more than just the pgd address, so we mask off what we don't want
			pml4_ptr &= ~0xFFF; 
			//pml4_ptr = TO_VIRT_ADDR(pml4_ptr);
			pml4_dir = (pml4_dir_t*) TO_VIRT_ADDR(pml4_ptr);
			cur_pdp_dir = alloc_new_frame();
			cur_pd_dir = alloc_new_frame();
			cur_pt_dir = alloc_new_frame();
		}
	}
}
