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
	namespace Allocation {
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
				//Make sure next slab is same permissions as next 
				if (slab_to_free->is_readonly == slab_to_free->next->is_readonly
				&& slab_to_free->is_executable == slab_to_free->next->is_executable) {
					slab_t* old_next = slab_to_free->next;
					slab_to_free->next = slab_to_free->next->next;
					slab_to_free->next->previous = slab_to_free;
					//We're removing a slab entry so we add the size of the entry to the free size
					slab_to_free->size += old_next->size + sizeof(slab_t);
					slab_to_free->is_readonly = old_next->is_readonly;
					slab_to_free->is_executable = old_next->is_executable;
				}
			}
			// Coallesce with previous slab is both are free
			if (slab_to_free != user_slab_head && slab_to_free->previous->is_free) {
				//Make sure next slab is same permissions as previous
				if (slab_to_free->is_readonly == slab_to_free->previous->is_readonly
				&& slab_to_free->is_executable == slab_to_free->previous->is_executable) {
					slab_t* rhs = slab_to_free->next;
					slab_t* lhs = slab_to_free->previous;
					
					rhs->previous = lhs;
					lhs->next = rhs;
					//We're removing a slab entry so we add the size of the entry to the free size
					lhs->size += slab_to_free->size + sizeof(slab_t);
					lhs->is_readonly = slab_to_free->is_readonly;
					lhs->is_executable = slab_to_free->is_executable;
				}
			}
			// TODO: invalidate page?
		}

		void kfree(void* vaddr) {
			slab_t* slab_to_free = vaddr - sizeof(slab_t);
			slab_to_free->is_free = true;
			// If next slab is free, coallesce with current free slab.
			if (slab_to_free->next != slab_head && slab_to_free->next->is_free) {
				//Make sure next slab is same permissions as next
				if (slab_to_free->is_readonly == slab_to_free->next->is_readonly
				&& slab_to_free->is_executable == slab_to_free->next->is_executable) {
					slab_t* old_next = slab_to_free->next;

					slab_to_free->next = slab_to_free->next->next;
					slab_to_free->next->previous = slab_to_free;

					//We're removing a slab entry so we add the size of the entry to the free size
					slab_to_free->size += old_next->size + sizeof(slab_t);
					slab_to_free->is_readonly = old_next->is_readonly;
					slab_to_free->is_executable = old_next->is_executable;
				}
			}
			// Coallesce with previous slab is both are free
			if (slab_to_free != slab_head && slab_to_free->previous->is_free) {
				//Make sure next slab is same permissions as previous
				if (slab_to_free->is_readonly == slab_to_free->previous->is_readonly
				&& slab_to_free->is_executable == slab_to_free->previous->is_executable) {
					slab_t* rhs = slab_to_free->next;
					slab_t* lhs = slab_to_free->previous;
					
					rhs->previous = lhs;
					lhs->next = rhs;
					//We're removing a slab entry so we add the size of the entry to the free size
					lhs->size += slab_to_free->size + sizeof(slab_t);
					lhs->is_readonly = slab_to_free->is_readonly;
					lhs->is_executable = slab_to_free->is_executable;
				}
			}
			// TODO: invalidate page?
			// 	Yes, because when we have processes, we want to be able to reuse unused kernel 
			// 	pages in user space
		}


		slab_t* find_last_user_slab() {
			return user_slab_head->previous;
		}

		slab_t* find_last_slab() {
			return slab_head->previous;
		}

		slab_t* create_new_pages(uint64_t num_pages, bool readonly, bool executable, bool isuser) {
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
						pt_entry_t* page = Memory::VMM::lookup_address(virt_addr);
						if (page == nullptr || (!page->present && page->phys_addr == 0x0)) {
							pages_to_allocate[i] = {
								.phys_addr = page_start, 
								.virt_addr = virt_addr, 
								.is_readonly = readonly, 
								.is_user = isuser, 
								.is_executable = executable 
							};
						} else {
							found_contiguous_pages = false;
							break;
						}
					}

					if (found_contiguous_pages) break;
					cur += slab_size;
				}

				for (auto frame : pages_to_allocate) {
					Memory::VMM::create_page_table_entry(&frame, Memory::VMM::pml4_dir);
					pt_entry_t* page = Memory::VMM::lookup_address(frame.virt_addr);
					page->present = true;
				}

				// Escape the for loop if we've found our pages/slab
				if (found_contiguous_pages) break;

				iter++;
			}

			// Write a new free slab entry at the beginning of new page range
			slab_t* slab_addr = (slab_t*)((uint8_t*)(pages_to_allocate[0].virt_addr));
			slab_t s = { 
				.next = slab_addr, 
				.previous = slab_addr, 
				.size = (slab_size - sizeof(slab_t)), 
				.is_free = true, 
				.is_readonly = readonly, 
				.is_executable = executable 
			};
			// Copy the slab entry to the beginning of the first page

			*slab_addr = s;

			//Insert into list
			if (slab_head != nullptr) {
				// Update last slab to point to new page's slab_t, only if this isn't the first slab
				// on the first page
				slab_t* last_slab = find_last_slab();

				if (isuser) 
					last_slab = find_last_user_slab();

				last_slab->next = (slab_t*)slab_addr;
				((slab_t*)slab_addr)->previous = last_slab;
				((slab_t*)slab_addr)->next = slab_head;
				// Update the circular link so head->prev == new_slab
				if (isuser) {
					user_slab_head->previous = slab_addr;
				} else {
					slab_head->previous = slab_addr;
				}
			}

			return (slab_t*)slab_addr;
		}

		slab_t* find_free_slab(uint64_t size, bool readonly, bool executable, bool isuser) {
			slab_t* cur = slab_head;
			if (isuser)
				cur = user_slab_head;

			if (cur->next != slab_head) {
				while (cur->next != slab_head) {
					if (cur->is_free && cur->size >= size
						&& cur->is_readonly == readonly 
						&& cur->is_executable == executable) break;
					cur = cur->next;
				}
			}
			if (cur->size >= size) {
				// we have to check is_free here b/c edge case of slab->size >= requested size, 
				// but is in use and is the last node in the list; basically we don't want the tail
				// to fall through
				if (cur->is_free && cur->is_readonly == readonly && cur->is_executable == executable) {
					return cur;
				} 
			}
			return nullptr;
		}

		void* uallocate(uint64_t objsize, uint64_t num, bool readonly, bool executable, bool align=false) {
			//TODO: Should probably refresh our pml4_dir ptr
			uint64_t numbytes = objsize * num;
			slab_t* slab = nullptr;
			if (align) {
				//- If space left in last slab >= sizeof(slab_t):
				//	- Allocate new page and coallesce that slab into the last slab
				//	- bifurcate slab at end_of_slab - page_size - sizeof(slab_t)
				//	- Put new slab_t at that address, and then data space starts at page boundary!
				//- If not enough space left in last slab:
				//	- Allocate 2 new pages
				//	- Bifurcate slab at middle boundary - sizeof(slab_t)
				//	- Coallesce left hand side slab with its neighbor, if you want, or just mark free
				//	- Put slab_t at middle_boundary - sizeof(slab_t)

				slab_t* last_slab = find_last_user_slab();
				//Space left in last slab and its free
				if (last_slab->size >= sizeof(slab_t) && last_slab->is_free) {
					//Its aligned, so we just assume its divisible
					uint64_t num_pages = (numbytes / PAGE_SIZE_BYTES);
					slab_t* new_page = create_new_pages(num_pages, false, false, false);
					//Looks like:
					//|(slab_t)-------|(slab_t)new_page---------------|
					
					//Bifurcate last_slab into last_slab->size - sizeof(slab_t) and sizeof(slab_t)
					uint64_t lhs_old_size = last_slab->size;
					uint64_t lhs_new_size = lhs_old_size - sizeof(slab_t);
					uint64_t rhs_new_size = 0;
					slab_t rhs = {
						.next = new_page,
						.previous = last_slab,
						.size = rhs_new_size,
						.is_free = true,
						.is_readonly = new_page->is_readonly,
						.is_executable = new_page->is_executable
					};
					slab_t* new_slab_addr = (slab_t*)((uint8_t*)last_slab + sizeof(slab_t) + lhs_new_size);
					last_slab->size = lhs_new_size;
					*new_slab_addr = rhs;

					last_slab->next = new_slab_addr;
					new_page->previous = new_slab_addr;

					//Now looks like:
					//|(slab_t)---|(slab_t)|(slab_t)-------------------|

					//Coallesce the new small slab with new_page, keeping the lhs slab entry
					new_slab_addr->size = new_page->size + sizeof(slab_t);
					new_slab_addr->next = new_page->next;
					user_slab_head->previous = new_slab_addr;
					
					//Now looks like:
					//|(slab_t)---|(slab_t)|---------------------------|
					//                     ^ page boundary

					slab = new_slab_addr;
				} else {
					//Its aligned, so we just assume its divisible
					uint64_t num_pages = (numbytes / PAGE_SIZE_BYTES) + 1;
					slab_t* new_page = create_new_pages(num_pages, false, false, false);
					uint64_t new_page_orig_size = new_page->size;
					
					//Bifurcate last_slab into last_slab->size - sizeof(slab_t) and sizeof(slab_t)
					uint64_t lhs_new_size = PAGE_SIZE_BYTES - sizeof(slab_t);
					uint64_t rhs_new_size = new_page->size - lhs_new_size - sizeof(slab_t);

					slab_t rhs = {
						.next = new_page->next,
						.previous = new_page,
						.size = rhs_new_size,
						.is_free = true,
						.is_readonly = new_page->is_readonly,
						.is_executable = new_page->is_executable
					};

					slab_t* rhs_addr = (slab_t*)((uint8_t*)new_page + sizeof(slab_t) + lhs_new_size);
					new_page->size = lhs_new_size;
					*rhs_addr = rhs;
					new_page->next = rhs_addr;
					user_slab_head->previous = rhs_addr;

					//Now we have:
					//|(slab_t)-------------|(slab_t)-----------------|...
					//Move second slab_t to other side, so we bifurcate again

					uint64_t new_page_old_size = new_page->size;
					uint64_t new_page_new_size = new_page_old_size - sizeof(slab_t);
					uint64_t new_rhs_new_size = 0;

					slab_t new_rhs = {
						.next = new_page->next,
						.previous = new_page,
						.size = new_rhs_new_size,
						.is_free = true,
						.is_readonly = new_page->is_readonly,
						.is_executable = new_page->is_executable
					};
					slab_t* new_rhs_addr = (slab_t*)((uint8_t*)new_page + sizeof(slab_t) + new_page_new_size);
					new_page->size = new_page_new_size;
					*new_rhs_addr = new_rhs;

					rhs_addr->previous = new_rhs_addr;
					new_page->next = new_rhs_addr;

					
					//Now we have:
					//|(slab_t)---------|(slab_t)|(slab_t)-----------------|...
					//Now modify new_rhs_addr->size to go to the end of the originally allocated space
					
					new_rhs_addr->size = new_rhs_addr->next->size + sizeof(slab_t);
					new_rhs_addr->next = new_rhs_addr->next->next;
					user_slab_head->previous = new_rhs_addr;
					
					//Finally we have:
					//|(slab_t)---------|(slab_t)|-----------------|...
					//                           ^ page boundary
					slab = new_rhs_addr;
				}
			} else {
				slab = find_free_slab(numbytes, readonly, executable, true);
				// Create new page if there is no free slab of correct size
				if (slab == nullptr || slab->size < numbytes) {
					// We need to make sure the aggr page size is big enough for both the object and
					// the slab_t's
					uint64_t num_pages = ((numbytes + sizeof(slab_t)) / PAGE_SIZE_BYTES);
					// Round up if not evenly divisible by page size
					if (((numbytes + sizeof(slab_t))) % PAGE_SIZE_BYTES != 0) 
						num_pages++;

					slab = create_new_pages(num_pages, readonly, executable, true);
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
						new_slab->is_readonly = slab->is_readonly;
						new_slab->is_executable = slab->is_executable;
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
			}
			// Set the slab as used;
			slab->is_free = false;

			return (void*) slab + sizeof(slab_t);
		}

		void* kallocate(uint64_t objsize, uint64_t num, bool align=false) {
			//TODO: Should probably refresh our pml4_dir ptr
			uint64_t numbytes = objsize * num;
			//This is the slab we're gonna return
			slab_t* slab = nullptr;
			if (align) {
				//- If space left in last slab >= sizeof(slab_t):
				//	- Allocate new page and coallesce that slab into the last slab
				//	- bifurcate slab at end_of_slab - page_size - sizeof(slab_t)
				//	- Put new slab_t at that address, and then data space starts at page boundary!
				//- If not enough space left in last slab:
				//	- Allocate 2 new pages
				//	- Bifurcate slab at middle boundary - sizeof(slab_t)
				//	- Coallesce left hand side slab with its neighbor, if you want, or just mark free
				//	- Put slab_t at middle_boundary - sizeof(slab_t)

				slab_t* last_slab = find_last_slab();
				//Space left in last slab and its free
				if (last_slab->size >= sizeof(slab_t) && last_slab->is_free) {
					//Its aligned, so we just assume its divisible
					uint64_t num_pages = (numbytes / PAGE_SIZE_BYTES);
					slab_t* new_page = create_new_pages(num_pages, false, false, false);
					//Looks like:
					//|(slab_t)-------|(slab_t)new_page---------------|
					
					//Bifurcate last_slab into last_slab->size - sizeof(slab_t) and sizeof(slab_t)
					uint64_t lhs_old_size = last_slab->size;
					uint64_t lhs_new_size = lhs_old_size - sizeof(slab_t);
					uint64_t rhs_new_size = 0;
					slab_t rhs = {
						.next = new_page,
						.previous = last_slab,
						.size = rhs_new_size,
						.is_free = true,
						.is_readonly = new_page->is_readonly,
						.is_executable = new_page->is_executable
					};

					slab_t* new_slab_addr = (slab_t*)((uint8_t*)last_slab + sizeof(slab_t) + lhs_new_size);
					last_slab->size = lhs_new_size;
					*new_slab_addr = rhs;

					last_slab->next = new_slab_addr;
					new_page->previous = new_slab_addr;

					//Now looks like:
					//|(slab_t)---|(slab_t)|(slab_t)-------------------|

					//Coallesce the new small slab with new_page, keeping the lhs slab entry
					new_slab_addr->size = new_page->size + sizeof(slab_t);
					new_slab_addr->next = new_page->next;
					new_page->next->previous = new_slab_addr;
					//slab_head->previous = new_slab_addr;
					
					//Now looks like:
					//|(slab_t)---|(slab_t)|---------------------------|
					//                     ^ page boundary

					slab = new_slab_addr;
				} else {
					//Its aligned, so we just assume its divisible
					uint64_t num_pages = (numbytes / PAGE_SIZE_BYTES) + 1;
					slab_t* new_page = create_new_pages(num_pages, false, false, false);
					uint64_t new_page_orig_size = new_page->size;
					
					//Bifurcate last_slab into last_slab->size - sizeof(slab_t) and sizeof(slab_t)
					uint64_t lhs_new_size = PAGE_SIZE_BYTES - sizeof(slab_t);
					uint64_t rhs_new_size = new_page->size - lhs_new_size - sizeof(slab_t);

					slab_t rhs = {
						.next = new_page->next,
						.previous = new_page,
						.size = rhs_new_size,
						.is_free = true,
						.is_readonly = new_page->is_readonly,
						.is_executable = new_page->is_executable
					};

					slab_t* rhs_addr = (slab_t*)((uint8_t*)new_page + sizeof(slab_t) + lhs_new_size);
					new_page->size = lhs_new_size;
					*rhs_addr = rhs;
					new_page->next = rhs_addr;
					slab_head->previous = rhs_addr;

					//Now we have:
					//|(slab_t)-------------|(slab_t)-----------------|...
					//Move second slab_t to other side, so we bifurcate again

					uint64_t new_page_old_size = new_page->size;
					uint64_t new_page_new_size = new_page_old_size - sizeof(slab_t);
					uint64_t new_rhs_new_size = 0;

					slab_t new_rhs = {
						.next = new_page->next,
						.previous = new_page,
						.size = new_rhs_new_size,
						.is_free = true,
						.is_readonly = new_page->is_readonly,
						.is_executable = new_page->is_executable
					};
					slab_t* new_rhs_addr = (slab_t*)((uint8_t*)new_page + sizeof(slab_t) + new_page_new_size);
					new_page->size = new_page_new_size;
					*new_rhs_addr = new_rhs;

					rhs_addr->previous = new_rhs_addr;
					new_page->next = new_rhs_addr;

					
					//Now we have:
					//|(slab_t)---------|(slab_t)|(slab_t)-----------------|...
					//Now modify new_rhs_addr->size to go to the end of the originally allocated space
					
					new_rhs_addr->size = new_rhs_addr->next->size + sizeof(slab_t);
					new_rhs_addr->next = new_rhs_addr->next->next;
					slab_head->previous = new_rhs_addr;
					
					//Finally we have:
					//|(slab_t)---------|(slab_t)|-----------------|...
					//                           ^ page boundary
					slab = new_rhs_addr;
				}
			} else {
				slab = find_free_slab(numbytes, false, false, false);
				// Create new page if there is no free slab of correct size
				if (slab == nullptr || slab->size < numbytes) {
					// We need to make sure the aggr page size is big enough for both the object and
					// the slab_t's
					uint64_t num_pages = ((numbytes + sizeof(slab_t)) / PAGE_SIZE_BYTES);
					// Round up if not evenly divisible by page size
					if (((numbytes + sizeof(slab_t))) % PAGE_SIZE_BYTES != 0) 
						num_pages++;

					slab = create_new_pages(num_pages, false, false, false);
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
						new_slab->is_readonly = slab->is_readonly;
						new_slab->is_executable = slab->is_executable;
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
					slab->is_readonly = false;
					slab->is_executable = false;
				}
			}
			// Set the slab as used;
			slab->is_free = false;

			return (void*) slab + sizeof(slab_t);
		}

		void init() {
			//Initialize our page tables, etc
			Memory::VMM::init();
			//Allocate the first page and set the slab_head
			slab_head = create_new_pages(1, false, false, false);
			user_slab_head = nullptr;
			dump_slab_list();
		}
		// Wrappers around the Memory::Paging functions to allow a more C-like calling method
		void* kalloc(uint64_t objsize, uint64_t num) {
			return Memory::Allocation::kallocate(objsize, num, false);
		}
		
		void* ualloc(uint64_t sizebytes, bool readonly, bool executable) {
			return Memory::Allocation::uallocate(sizebytes, 1, readonly, executable, false);
		}
	}
}

