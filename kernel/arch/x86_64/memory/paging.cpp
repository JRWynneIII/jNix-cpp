#include<cstddef>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<kernel.h>
#include<kernel/memory.h>
#include<kernel/ptr.hpp>
#include<cstdint>

#define SLAB_BUFFER_BYTES 10

namespace Memory {
	namespace Paging {
		pml4_dir_t* pml4_dir;
		pdp_dir_t*  cur_pdp_dir;
		pd_dir_t*   cur_pd_dir;
		pt_dir_t*   cur_pt_dir;

		//TODO: I don't like this, collapse user slabs into 1 slab tree
		slab_t* slab_head;
		slab_t* get_slab_head() { return slab_head; }

		//User space will have a separate slab tree, maybe?
		slab_t* user_slab_head;
		slab_t* get_user_slab_head() { return user_slab_head; }
		
		//TODO: Add page fault handler, on page fault, repair slab table
		//Attempts to walk the table, and ensures that sizes match slabs. If sizes don't match, then we can
		//assume that there was a buffer overrun that broke something
		uint64_t repair_slab_table() {
			slab_t* cur = slab_head;
		}

		void ufree(void* vaddr) {
			slab_t* slab_to_free = vaddr - sizeof(slab_t);
			slab_to_free->is_free = true;
			// If next slab is free, coallesce with current free slab.
			if (slab_to_free->next != user_slab_head && slab_to_free->next->is_free) {
				slab_t* old_next = slab_to_free->next;
				slab_to_free->next = slab_to_free->next->next;
				slab_to_free->next->previous = slab_to_free;
				//We're removing a slab entry so we add the size of the entry to the free size
				slab_to_free->size += old_next->size + sizeof(slab_t);
			}
			// Coallesce with previous slab is both are free
			if (slab_to_free != user_slab_head && slab_to_free->previous->is_free) {
				slab_t* rhs = slab_to_free->next;
				slab_t* lhs = slab_to_free->previous;
				
				rhs->previous = lhs;
				lhs->next = rhs;
				//We're removing a slab entry so we add the size of the entry to the free size
				lhs->size += slab_to_free->size + sizeof(slab_t);
			}
			// TODO: invalidate page?
		}

		void kfree(void* vaddr) {
			slab_t* slab_to_free = vaddr - sizeof(slab_t);
			slab_to_free->is_free = true;
			// If next slab is free, coallesce with current free slab.
			if (slab_to_free->next != slab_head && slab_to_free->next->is_free) {
				slab_t* old_next = slab_to_free->next;

				slab_to_free->next = slab_to_free->next->next;
				slab_to_free->next->previous = slab_to_free;

				//We're removing a slab entry so we add the size of the entry to the free size
				slab_to_free->size += old_next->size + sizeof(slab_t);
			}
			// Coallesce with previous slab is both are free
			if (slab_to_free != slab_head && slab_to_free->previous->is_free) {
				slab_t* rhs = slab_to_free->next;
				slab_t* lhs = slab_to_free->previous;
				
				rhs->previous = lhs;
				lhs->next = rhs;
				//We're removing a slab entry so we add the size of the entry to the free size
				lhs->size += slab_to_free->size + sizeof(slab_t);
			}
			// TODO: invalidate page?
			// 	Yes, because when we have processes, we want to be able to reuse unused kernel 
			// 	pages in user space
		}

		uintptr_t alloc_new_frame() {
			uintptr_t base = Memory::usable_memory_regions[0].base;
			// Don't override last_dir_alloc if its already set
			static uintptr_t end_dir_addr = end_dir_addr ? end_dir_addr : base;

			void* old_base = end_dir_addr;
			end_dir_addr += 4096;
			return old_base; 
		}

		uintptr_t alloc_new_table_or_next_entry(page_map_u table) {
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
		
		pml4_entry_t* find_or_create_pml4_entry(pml4_entry_t* address, uint64_t offset) {
			pml4_entry_t* entry = (pml4_entry_t*)(address + offset);
			if (!entry->present) {
				page_map_u table;
				table.pdp = cur_pdp_dir;
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

		pdp_entry_t* find_or_create_pdp_entry(uintptr_t address, uint64_t offset) {
			address = TO_VIRT_ADDR(address);
			pdp_entry_t* entry = ((pdp_dir_t*)address)->dir + offset;
			if (!entry->present) {
				page_map_u table;
				table.pd = cur_pd_dir;
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

		pd_entry_t* find_or_create_pd_entry(uintptr_t address, uint64_t offset) {
			address = TO_VIRT_ADDR(address);
			pd_entry_t* entry = ((pd_dir_t*)address)->dir + offset;
			if (!entry->present) {
				page_map_u table;
				table.pt = cur_pt_dir;
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

		pt_entry_t* find_or_create_pt_entry(uintptr_t address, uint64_t offset, uintptr_t phys_addr, bool readonly, bool isuser) {
			address = TO_VIRT_ADDR(address);
			pt_entry_t* entry = ((pt_dir_t*)address)->pages + offset;
			if (!entry->present) {
				//create entry
				pt_entry_t new_entry;
				new_entry.present = 1;
				new_entry.rw = 1;
				new_entry.phys_addr = phys_addr;
				if (isuser) new_entry.user = 1;
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
		void create_page_table_entry(frame_t* f) {
			//find offsets into each table
			// all offsets can be calculated from the vaddr
			uint64_t pml4_offset = ((f->virt_addr >> 12) >> 27) & 0x1FF;
			uint64_t pdp_offset = ((f->virt_addr >> 12) >> 18) & 0x1FF;
			uint64_t pd_offset = ((f->virt_addr >> 12) >> 9) & 0x1FF;
			uint64_t pt_offset = (f->virt_addr >> 12) & 0x1FF;

			uint64_t phys_offset = f->virt_addr & 0xFFF;

			pml4_entry_t* pml = find_or_create_pml4_entry(pml4_dir->dir, pml4_offset);
			pdp_entry_t* pdp = find_or_create_pdp_entry(pml->pdp_address, pdp_offset);
			pd_entry_t* pd = find_or_create_pd_entry(pdp->pd_address, pd_offset);
			pt_entry_t* pt = find_or_create_pt_entry(pd->pt_address, pt_offset, f->phys_addr, f->is_readonly, f->is_user);
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

		slab_t* find_last_user_slab() {
			return user_slab_head->previous;
		}

		slab_t* find_last_slab() {
			return slab_head->previous;
		}

		//TODO: Collapse this into create_new_pages
		slab_t* create_new_user_pages(uint64_t num_pages, bool readonly) {
			uint64_t iter = 0;
			// When we allocate n new pages, slab_size is going to be less than the full page since we keep
			// slab_t there
			uint64_t slab_size = num_pages * PAGE_SIZE_BYTES;
			
			frame_t pages_to_allocate[num_pages];
			for (auto region : Memory::usable_memory_regions) {
				// Skip first memory region since it has page directory stuff there
				if (iter == 0) {
					iter++;
					continue;
				}

				uintptr_t cur = region.base;
				// Flag for finding the correct number of contiguous pages
				bool found_contiguous_pages = true;
				//while(cur < (region.base + region.length)) {
				while(cur < ((region.base + region.length) - slab_size)) {
					found_contiguous_pages = true;
					// Look for num_pages of contiguous pages
					for (int i = 0; i < num_pages; i++) {
						uintptr_t page_start = cur + (PAGE_SIZE_BYTES * i);
						//Get the virtual address for the physical address of the beginning of this slab
						uintptr_t virt_addr = TO_VIRT_ADDR(page_start);
						pt_entry_t* page = lookup_address(virt_addr);
						if (page == nullptr || (!page->present && page->phys_addr == 0x0)) {
							pages_to_allocate[i] = {page_start, virt_addr, readonly, true};
						} else {
							found_contiguous_pages = false;
							break;
						}
					}

					if (found_contiguous_pages) break;
					cur += slab_size;
				}

				for (auto frame : pages_to_allocate) {
					create_page_table_entry(&frame);
					pt_entry_t* page = lookup_address(frame.virt_addr);
					page->present = true;
				}

				// Escape the for loop if we've found our pages/slab
				if (found_contiguous_pages) break;

				iter++;
			}
			// Write a new free slab entry at the beginning of new page range
			slab_t* slab_addr = (slab_t*)((uint8_t*)(pages_to_allocate[0].virt_addr));
			slab_t s = { slab_addr, slab_addr, (slab_size - sizeof(slab_t)), true, readonly };
			// Copy the slab entry to the beginning of the first page

			*slab_addr = s;

			//Insert into list
			if (user_slab_head != nullptr) {
				// Update last slab to point to new page's slab_t, only if this isn't the first slab
				// on the first page
				slab_t* last_slab = find_last_user_slab();
				last_slab->next = (slab_t*)slab_addr;
				((slab_t*)slab_addr)->previous = last_slab;
				((slab_t*)slab_addr)->next = slab_head;
				// Update the circular link so head->prev == new_slab
				slab_head->previous = slab_addr;
			}

			return (slab_t*)slab_addr;
		}

		slab_t* create_new_pages(uint64_t num_pages) {
			uint64_t iter = 0;
			// When we allocate n new pages, slab_size is going to be less than the full page since we keep
			// slab_t there
			uint64_t slab_size = num_pages * PAGE_SIZE_BYTES;
			
			frame_t pages_to_allocate[num_pages];
			for (auto region : Memory::usable_memory_regions) {
				// Skip first memory region since it has page directory stuff there
				if (iter == 0) {
					iter++;
					continue;
				}

				uintptr_t cur = region.base;
				// Flag for finding the correct number of contiguous pages
				bool found_contiguous_pages = true;
				//while(cur < (region.base + region.length)) {
				while(cur < ((region.base + region.length) - slab_size)) {
					found_contiguous_pages = true;
					// Look for num_pages of contiguous pages
					for (int i = 0; i < num_pages; i++) {
						uintptr_t page_start = cur + (PAGE_SIZE_BYTES * i);
						//Get the virtual address for the physical address of the beginning of this slab
						uintptr_t virt_addr = TO_VIRT_ADDR(page_start);
						pt_entry_t* page = lookup_address(virt_addr);
						if (page == nullptr || (!page->present && page->phys_addr == 0x0)) {
							pages_to_allocate[i] = {page_start, virt_addr, false, false};
						} else {
							found_contiguous_pages = false;
							break;
						}
					}

					if (found_contiguous_pages) break;
					cur += slab_size;
				}

				for (auto frame : pages_to_allocate) {
					create_page_table_entry(&frame);
					pt_entry_t* page = lookup_address(frame.virt_addr);
					page->present = true;
				}

				// Escape the for loop if we've found our pages/slab
				if (found_contiguous_pages) break;

				iter++;
			}

			// Write a new free slab entry at the beginning of new page range
			slab_t* slab_addr = (slab_t*)((uint8_t*)(pages_to_allocate[0].virt_addr));
			slab_t s = { slab_addr, slab_addr, (slab_size - sizeof(slab_t)), true, false };
			// Copy the slab entry to the beginning of the first page

			*slab_addr = s;

			//Insert into list
			if (slab_head != nullptr) {
				// Update last slab to point to new page's slab_t, only if this isn't the first slab
				// on the first page
				slab_t* last_slab = find_last_slab();
				last_slab->next = (slab_t*)slab_addr;
				((slab_t*)slab_addr)->previous = last_slab;
				((slab_t*)slab_addr)->next = slab_head;
				// Update the circular link so head->prev == new_slab
				slab_head->previous = slab_addr;
			}

			return (slab_t*)slab_addr;
		}

		slab_t* find_free_slab(uint64_t size) {
			slab_t* cur = slab_head;
			if (cur->next != slab_head) {
				while (cur->next != slab_head) {
					if (cur->is_free && cur->size >= size) {
						break;
					}
					cur = cur->next;
				}
			}
			if (cur->size >= size) {
				// we have to check is_free here b/c edge case of slab->size >= requested size, 
				// but is in use and is the last node in the list; basically we don't want the tail
				// to fall through
				if (cur->is_free) {
					return cur;
				} 
			}
			return nullptr;
		}

		//TODO: Collapse this into find_free_slab
		slab_t* find_free_user_slab(uint64_t size, bool readonly) {
			slab_t* cur = user_slab_head;
			if (cur->next != user_slab_head) {
				while (cur->next != user_slab_head) {
					if (cur->is_free && cur->size >= size && cur->is_readonly == readonly) {
						break;
					}
					cur = cur->next;
				}
			}
			if (cur->size >= size) {
				// we have to check is_free here b/c edge case of slab->size >= requested size, 
				// but is in use and is the last node in the list; basically we don't want the tail
				// to fall through
				if (cur->is_free && cur->is_readonly == readonly) {
					return cur;
				} 
			}
			return nullptr;
		}

		void* ualloc(uint64_t objsize, uint64_t num, bool readonly) {
			//TODO: Should probably refresh our pml4_dir ptr
			uint64_t numbytes = objsize * num;
			slab_t* slab = find_free_user_slab(numbytes, readonly);
			// Create new page if there is no free slab of correct size
			if (slab == nullptr || slab->size < numbytes) {
				// We need to make sure the aggr page size is big enough for both the object and
				// the slab_t's
				uint64_t num_pages = ((numbytes + sizeof(slab_t)) / PAGE_SIZE_BYTES);
				// Round up if not evenly divisible by page size
				if (((numbytes + sizeof(slab_t))) % PAGE_SIZE_BYTES != 0) 
					num_pages++;

				slab = create_new_user_pages(num_pages, readonly);
			}

			if (slab->size > numbytes) {
				// If requested slab is smaller than found slab, then bifurcate the slab
				uint64_t old_slab_orig_size = slab->size;
				uint64_t old_slab_orig_full_size = old_slab_orig_size + sizeof(slab_t);

				uint64_t old_slab_new_size = numbytes;
				uint64_t old_slab_new_full_size = old_slab_new_size + sizeof(slab_t);

				int64_t new_slab_full_size = old_slab_orig_full_size - numbytes - sizeof(slab_t);
				int64_t new_slab_size = new_slab_full_size - sizeof(slab_t);

				// We have a possibility to have a slab that is < sizeof(slab_t) after bifurcation
				// So to avoid this, we just won't bifurcate if the remaining slab isn't going to fit right
				if (new_slab_size > 0) {
					slab->size = numbytes;

					slab_t* new_slab = (slab_t*)((uint8_t*)(slab) + sizeof(slab_t) + numbytes);
					new_slab->next = slab->next;
					new_slab->previous = slab;
					new_slab->size = new_slab_size;
					new_slab->is_free = true;

					//Insert into list
					slab->next->previous = new_slab;
					slab->next = new_slab;
				}
			} else if (slab->size < numbytes) {
				logfk(ERROR, "Cannot allocate larger object in smaller slab!\n");
				printfk("Slab size: %d\tnumbytes: %d\n", slab->size, numbytes);
				halt();
			} else {
				slab->size = numbytes;
			}
			// Set the slab as used;
			slab->is_free = false;

			return (void*) slab + sizeof(slab_t);
		}

		void* kalloc(uint64_t objsize, uint64_t num) {
			//TODO: Should probably refresh our pml4_dir ptr
			uint64_t numbytes = objsize * num;
			slab_t* slab = find_free_slab(numbytes);
			// Create new page if there is no free slab of correct size
			if (slab == nullptr || slab->size < numbytes) {
				// We need to make sure the aggr page size is big enough for both the object and
				// the slab_t's
				uint64_t num_pages = ((numbytes + sizeof(slab_t)) / PAGE_SIZE_BYTES);
				// Round up if not evenly divisible by page size
				if (((numbytes + sizeof(slab_t))) % PAGE_SIZE_BYTES != 0) 
					num_pages++;

				slab = create_new_pages(num_pages);
			}

			if (slab->size > numbytes) {
				// If requested slab is smaller than found slab, then bifurcate the slab
				//                      100
				// |slab_t|----data----------------------------------------
				// 		10		   90-(sizeof(slab_t)) = 52
				// |slab_t|----data----|slab_t|----data---------------------

				uint64_t old_slab_orig_size = slab->size;
				uint64_t old_slab_orig_full_size = old_slab_orig_size + sizeof(slab_t);

				uint64_t old_slab_new_size = numbytes;
				uint64_t old_slab_new_full_size = old_slab_new_size + sizeof(slab_t);

				int64_t new_slab_full_size = old_slab_orig_full_size - numbytes - sizeof(slab_t);
				int64_t new_slab_size = new_slab_full_size - sizeof(slab_t);

				// We have a possibility to have a slab that is < sizeof(slab_t) after bifurcation
				// So to avoid this, we just won't bifurcate if the remaining slab isn't going to fit right
				if (new_slab_size > 0) {
					//slab_t new_slab = {slab->next, slab, (uint64_t)new_slab_size, true};

					slab->size = numbytes;

					// |slab_t|----data----|slab_t|----data----
					// ^slab  
					//        ^ slab+sizeof(slab_t)
					//                     ^ + slab->size
					// NOTE: If you're going to do pointer math, make sure you use bytes instead of
					// the default, which is the sizeof(type). Bleh. This caused SO MANY WEIRD ISSUES
					// since we keep track of size in bytes, but moved the pointer and place the new slab by 
					// O(sizeof(slab_t)) if we didn't cast 'slab' as uint8_t*. This is dumb
					slab_t* new_slab = (slab_t*)((uint8_t*)(slab) + sizeof(slab_t) + numbytes);
					new_slab->next = slab->next;
					new_slab->previous = slab;
					new_slab->size = new_slab_size;
					new_slab->is_free = true;

					//Insert into list
					slab->next->previous = new_slab;
					slab->next = new_slab;
				}
			} else if (slab->size < numbytes) {
				logfk(ERROR, "Cannot allocate larger object in smaller slab!\n");
				printfk("Slab size: %d\tnumbytes: %d\n", slab->size, numbytes);
				halt();
			} else {
				slab->size = numbytes;
			}
			// Set the slab as used;
			slab->is_free = false;

			return (void*) slab + sizeof(slab_t);
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
			//Allocate the first page and set the slab_head
			//slab_head = create_new_page();
			slab_head = create_new_pages(1);
			user_slab_head = nullptr;
		}
	}
}

// Wrappers around the Memory::Paging functions to allow a more C-like calling method
void* kalloc(uint64_t sizebytes) {
	return Memory::Paging::kalloc(sizebytes, 1);
}

void kfree(void* ptr) {
	Memory::Paging::kfree(ptr);
}

void* ualloc(uint64_t sizebytes, bool readonly) {
	return Memory::Paging::ualloc(sizebytes, 1, readonly);
}

void ufree(void* ptr) {
	Memory::Paging::ufree(ptr);
}
