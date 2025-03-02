#include<cstddef>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<kernel.h>
#include<memory.h>

namespace Memory {
	namespace Paging {
		pml4_dir_t* pml4_dir;
		slab_t* slab_head;
		slab_t* get_slab_head() { return slab_head; }
		
		//TODO: Add page fault handler, on page fault, repair slab table
		//Attempts to walk the table, and ensures that sizes match slabs. If sizes don't match, then we can
		//assume that there was a buffer overrun that broke something
		uint64_t repair_slab_table() {
			slab_t* cur = slab_head;
		}

		void kfree(void* vaddr) {
			slab_t* slab_to_free = vaddr - sizeof(slab_t);
			slab_to_free->is_free = true;
			// If next slab is free, coallesce with current free slab.
			if (slab_to_free->next != slab_head && slab_to_free->next->is_free) {
				slab_t* old_next = slab_to_free->next;

				slab_to_free->next = slab_to_free->next->next;
				slab_to_free->next->previous = slab_to_free;

				slab_to_free->size += old_next->size;
			}
			// Coallesce with previous slab is both are free
			if (slab_to_free != slab_head && slab_to_free->previous->is_free) {
				slab_t* rhs = slab_to_free->next;
				slab_t* lhs = slab_to_free->previous;
				
				rhs->previous = lhs;
				lhs->next = rhs;
				lhs->size += slab_to_free->size;
			}
			// TODO: invalidate page?
			// 	do we really need to do that when we are tracking free slabs?
		}

		uintptr_t alloc_dir_entry() {
			// TODO: This might cause page faults if pages aren't allocated for these addresses
			uintptr_t base = Memory::usable_memory_regions[0].base;
			// Don't override last_dir_alloc if its already set
			static uintptr_t end_dir_addr = end_dir_addr ? end_dir_addr : base;

			void* old_base = end_dir_addr;
			end_dir_addr += 64;
			return old_base; 
		}

		pml4_entry_t* find_or_create_pml4_entry(pml4_entry_t* address, uint64_t offset) {
			pml4_entry_t* entry = (pml4_entry_t*)(address + offset);
			if (!entry->present) {
				uintptr_t pdp_addr = alloc_dir_entry();
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
				uintptr_t pd_addr = alloc_dir_entry();
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
				uintptr_t pt_addr = alloc_dir_entry();
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

		pt_entry_t* find_or_create_pt_entry(uintptr_t address, uint64_t offset, uintptr_t phys_addr) {
			address = TO_VIRT_ADDR(address);
			pt_entry_t* entry = ((pt_dir_t*)address)->pages + offset;
			if (!entry->present) {
				//create entry
				pt_entry_t new_entry;
				new_entry.present = 0;
				new_entry.rw = 1;
				new_entry.phys_addr = phys_addr;
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

			//find the addresses necesary; may need to have them returned by each prev function
			pml4_entry_t* pml = find_or_create_pml4_entry(pml4_dir->dir, pml4_offset);
			pdp_entry_t* pdp = find_or_create_pdp_entry(pml->pdp_address, pdp_offset);
			pd_entry_t* pd = find_or_create_pd_entry(pdp->pd_address, pd_offset);
			pt_entry_t* pt = find_or_create_pt_entry(pd->pt_address, pt_offset, f->phys_addr);

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

		slab_t* find_last_slab() {
			return slab_head->previous;
		//	slab_t* cur = slab_head;
		//	while (cur->next != nullptr) {
		//		cur = cur->next;
		//	}
		//	return cur;
		}

		slab_t* create_new_pages(uint64_t num_pages) {
			uint64_t iter = 0;
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
				while(cur < (region.base + region.length)) {
					// Look for num_pages of contiguous pages
					for (int i = 0; i < num_pages; i++) {
						uintptr_t page_start = cur + (PAGE_SIZE_BYTES * i);
						//Get the virtual address for the physical address of the beginning of this slab
						uintptr_t virt_addr = TO_VIRT_ADDR(page_start);
						pt_entry_t* page = lookup_address(virt_addr);
						if (page == nullptr || (!page->present && page->phys_addr == 0x0)) {
							pages_to_allocate[i] = {page_start, virt_addr};
						} else {
							found_contiguous_pages = false;
							break;
						}
					}

					if (found_contiguous_pages) {
						for (auto frame : pages_to_allocate) {
							create_page_table_entry(&frame);
							pt_entry_t* page = lookup_address(frame.virt_addr);
							page->present = true;
						}
						// Escape from the while loop. We've found and allocated our pages
						break;
					}
					cur += slab_size;
				}

				// Escape the for loop if we've found our pages/slab
				if (found_contiguous_pages) break;

				iter++;
			}

			// Write a new free slab entry at the beginning of new page range
			uintptr_t slab_addr = pages_to_allocate[0].virt_addr;
			slab_t s = { slab_addr, slab_addr, slab_size, true };
			// Copy the slab entry to the beginning of the first page
			memcpy(slab_addr, &s, sizeof(slab_t));
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

		void* kalloc(uint64_t objsize, uint64_t num) {
			uint64_t numbytes = objsize * num;
			slab_t* slab = find_free_slab(numbytes);
			// Create new page if there is no free slab of correct size
			if (slab == nullptr || slab->size < numbytes) {
				//slab = create_new_page();
				uint64_t num_pages = (numbytes / PAGE_SIZE_BYTES);
				// Round up if not evenly divisible by page size
				if (numbytes % PAGE_SIZE_BYTES != 0) 
					num_pages++;

				slab = create_new_pages(num_pages);
			}

			if (slab->size > numbytes) {
				// If requested slab is smaller than found slab, then bifurcate the slab
				slab_t new_slab = {slab->next, slab, slab->size - numbytes, true};

				slab_t* new_slab_addr = slab + slab->size + sizeof(slab_t);

				slab->next->previous = new_slab_addr;
				slab->size = numbytes;
				slab->next = new_slab_addr;

				memcpy(new_slab_addr, &new_slab, sizeof(slab_t));
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
			//return flist[0].virt_addr;
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
			//Allocate the first page and set the slab_head
			//slab_head = create_new_page();
			slab_head = create_new_pages(1);
		}
	}
}
