#include<cstddef>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<jnix.h>

#define TO_VIRT_ADDR(a) ((uintptr_t)a | Memory::hhdm_offset)
#define PAGE_SIZE_BYTES 4096

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
	

struct frame_t {
	// Phys and virt addresses cached for convenience sake
	uintptr_t phys_addr;
	uintptr_t virt_addr;
};

typedef struct slab_t;
struct slab_t {
	slab_t* next;
	uint64_t size; //bytes
	bool is_free;
};

#define SLAB_PTR_SIZE sizeof(slab_t)
#define SLAB_START_ADDRESS(s) &s + SLAB_PTR_SIZE
#define SLAB_END_ADDRESS(s) SLAB_START_ADDRESS(s) + s->size

extern uint64_t kernelend;

#define IS_ALIGNED(a) (a & 0xFFF) == 0

namespace Memory {
	namespace Paging {
		pml4_dir_t* pml4_dir;

		slab_t* slab_head;

		void kfree(void* vaddr) {
			slab_t* slab_to_free = vaddr - sizeof(slab_t);
			slab_to_free->is_free = true;
			// If next slab is free, coallesce with current free slab.
			// TODO: Maybe change slabs to be doubly linked, circular? list so that you can coallesce with 
			// the previous slab
			if (slab_to_free->next->is_free) {
				slab_to_free->next = slab_to_free->next->next;
				slab_to_free->size += slab_to_free->next->size;
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

		// TODO: do a bounds check for < 512 for offset
		pml4_entry_t* find_or_create_pml4_entry(pml4_entry_t* address, uint64_t offset) {
			pml4_entry_t* entry = (pml4_entry_t*)(address + offset);
			if (!entry->present) {
				//If entry is marked not present, then we're missing a PDP for that address
				//TODO: This is broken on all find_or_create_*_entry's. If next level isn't present,
				//	we can't create a new one b/c we don't know where the end of the pdps are
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

		// TODO: do a bounds check for < 512 for offset
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

		// TODO: do a bounds check for < 512 for offset
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

		// TODO: do a bounds check for < 512 for offset
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
			pml4_entry_t* pml = find_or_create_pml4_entry(f, pml4_dir->dir, pml4_offset);
			pdp_entry_t* pdp = find_or_create_pdp_entry(f, pml->pdp_address, pdp_offset);
			pd_entry_t* pd = find_or_create_pd_entry(f, pdp->pd_address, pd_offset);
			pt_entry_t* pt = find_or_create_pt_entry(f, pd->pt_address, pt_offset, f->phys_addr);

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
			slab_t* cur = slab_head;
			while (cur->next != nullptr) {
				cur = cur->next;
			}
			return cur;
		}

		slab_t* create_multi_new_pages(uint64_t num_pages) {
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
							pages_to_allocate[i] = {page_start, virt_addr, true};
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
			slab_t s = { nullptr, slab_size, true };
			uintptr_t slab_addr = pages_to_allocate[0].virt_addr;
			// Copy the slab entry to the beginning of the first page
			memcpy(slab_addr, &s, sizeof(slab_t));
			if (slab_head != nullptr) {
				// Update last slab to point to new page's slab_t, only if this isn't the first slab
				// on the first page
				slab_t* last_slab = find_last_slab();
				last_slab->next = (slab_t*)slab_addr;
			}

			return (slab_t*)slab_addr;
		}
		slab_t* create_new_page() {
			uintptr_t paddr = 0x0;
			frame_t f;
			for (int i=0; i < 7 ; i++) {
				//Skip the first memory region as it will be used for page directory junk
				if (i == 0) {
					continue;
				}
				mem_region region = Memory::usable_memory_regions[i];
				uintptr_t cur = region.base;
				uintptr_t end = (region.base + region.length);
				while (cur < end) {
					uintptr_t virt_addr = TO_VIRT_ADDR(cur);
					pt_entry_t* page = lookup_address(virt_addr);
					// Page doesn't exist yet, so make the entries with create_page_table_entry
					// and don't reset cur, let it continue and it'll retest it 

					// If we don't get a page back OR we get a page entry that's been 0'd out
					if (page == nullptr || (!page->present && page->phys_addr == 0x0)) {
						f = {cur, virt_addr, true};
						create_page_table_entry(&f);
						page = lookup_address(virt_addr);
						if (page == nullptr) {
							continue;
						}
					}

					if (!page->present) {
						paddr = page->phys_addr;
						page->present = true;
						break;
					}

					cur = cur + PAGE_SIZE_BYTES;
				}

				if (paddr != 0x0) {
					break;
				}
			}

			uintptr_t vaddr = TO_VIRT_ADDR(paddr);
			f = {paddr, vaddr, true};
			//create_page_table_entry(&f);
			// Write a new free slab entry at the beginning of new page
			// TODO: This might not be a good idea, here. This might make us limited to PAGE_SIZE_BYTESbyte objects
			slab_t s = { nullptr, PAGE_SIZE_BYTES, true };
			memcpy(vaddr, &s, sizeof(slab_t));
			if (slab_head != nullptr) {
				// Update last slab to point to new page's slab_t, only if this isn't the first slab
				// on the first page
				slab_t* last_slab = find_last_slab();
				last_slab->next = (slab_t*)vaddr;
			}

			return (slab_t*)vaddr;
		}

		slab_t* find_free_slab(uint64_t size) {
			slab_t* cur = slab_head;
			while (cur->next != nullptr) {
				if (cur->is_free && cur->size >= size) {
					break;
				}
				cur = cur->next;
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

		void coallesce_all_free_slabs() {
			slab_t* cur = slab_head;
			while (cur->next != nullptr) {
				if (cur->is_free && cur->next->is_free) {
					cur->size += cur->next->size;
					cur->next = cur->next->next;
				}
				cur = cur->next;
			}
		}

		void coallesce_page_free_slabs(frame_t page) {
			// Start at the beginning of the page
			slab_t* cur = (slab_t*) page.virt_addr;
			while (cur->next != nullptr) {
				if (cur->is_free && cur->next->is_free) {
					cur->size += cur->next->size;
					cur->next = cur->next->next;
				}
				cur = cur->next;
				// Check if next slab starts on a different page
				if (cur >= page.virt_addr + PAGE_SIZE_BYTES) {
					break;
				}
			}
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

				printk("Allocating ");
				printk(itoa(num_pages));
				printk(" pages\n");

				slab = create_multi_new_pages(num_pages);
			}

			if (slab->size > numbytes) {
				// If requested slab is smaller than found slab, then bifurcate the slab
				slab_t new_slab = {slab->next, slab->size - numbytes, true};
				slab_t* new_slab_addr = slab + slab->size + sizeof(slab_t);
				memcpy(new_slab_addr, &new_slab, sizeof(slab_t));
				slab->size = numbytes;
				slab->next = new_slab_addr;
			} else if (slab->size < numbytes) {
				logk("Cannot allocate larger object in smaller slab!\n", ERROR);
				printk("Slab size: ");
				printk(itoa(slab->size));
				printk("\nnumbytes: ");
				printk(itoa(numbytes));
				printk("\n");
				for(;;);
			} else {
				slab->size = numbytes;
			}
			// Set the slab as used;
			slab->is_free = false;

			return (void*) slab + sizeof(slab_t);
			//return flist[0].virt_addr;
		}

		void test() {
			logk("Testing allocation of < PAGE_SIZE_BYTES bytes (5 * uint64_t) ", KERNEL);
			uint64_t* t1 = kalloc(sizeof(uint64_t), 5);
			printk(hex_to_str(t1));
			t1[0] = 1;
			t1[1] = 2;
			t1[2] = 3;
			t1[3] = 4;
			t1[4] = 5;
			bool pass = true;
			for (int i=0; i<5; i++) {
				if (t1[i] != i+1) pass = false;
			}
			if (pass) {
				printk(" PASSED\n");
			} else {
				printk(" FAILED\n");
			}
			logk("Testing allocation of = PAGE_SIZE_BYTES bytes ", KERNEL);
			uint64_t* t3 = kalloc(sizeof(uint64_t), 512);
			printk(hex_to_str(t3));
			for (int i = 0; i< 512; i++) {
				t3[i] = i+1;
			}
			pass = true;
			for (int i=0; i<512; i++) {
				if (t3[i] != i+1) {
					pass = false;
				}
			}

			if (pass) {
				printk(" PASSED\n");
			} else {
				printk(" FAILED\n");
			}

			logk("Testing allocation of < PAGE_SIZE_BYTES bytes (5 * uint64_t) (second pass) ", KERNEL);
			uint64_t* t2 = kalloc(sizeof(uint64_t), 5);
			printk(hex_to_str(t2));
			t2[0] = 6;
			t2[1] = 7;
			t2[2] = 8;
			t2[3] = 9;
			t2[4] = 10;
			pass = true;
			for (int i=0; i<5; i++) {
				if (t2[i] != i+6) pass = false;
			}

			if (pass) {
				printk(" PASSED\n");
			} else {
				printk(" FAILED\n");
			}

			logk("Testing allocation of = PAGE_SIZE_BYTES bytes (second pass) ", KERNEL);
			uint64_t* t4 = kalloc(sizeof(uint64_t), 512);
			printk(hex_to_str(t4));
			for (int i = 0; i< 512; i++) {
				t4[i] = i+1;
			}
			pass = true;
			for (int i=0; i<512; i++) {
				if (t4[i] != i+1) pass = false;
			}

			if (pass) {
				printk(" PASSED\n");
			} else {
				printk(" FAILED\n");
			}

			logk("Testing allocation of > PAGE_SIZE_BYTES bytes (5 * uint64_t)\n", KERNEL);
			uint64_t* t5 = kalloc(sizeof(uint64_t), 1024);
			printk(hex_to_str(t5));
			for (int i = 0; i< 1024; i++) {
				t5[i] = i+1;
			}
			pass = true;
			for (int i=0; i<1024; i++) {
				if (t5[i] != i+1) pass = false;
			}

			if (pass) {
				printk(" PASSED\n");
			} else {
				printk(" FAILED\n");
			}
		}

		void print_pml4_entry(pml4_entry_t* entry) {
			logk("PML4 entry: ", KERNEL);
			printk("present: ");
			printk(itoa(entry->present));
			printk(" rw: ");
			printk(itoa(entry->rw));
			printk(" user: ");
			printk(itoa(entry->user));
			printk(" write_through: ");
			printk(itoa(entry->write_through));
			printk(" cache_disabled: ");
			printk(itoa(entry->cache_disabled));
			printk(" accessed: ");
			printk(itoa(entry->accessed));
			printk(" ignored1: ");
			printk(itoa(entry->ignored1));
			printk(" page_size: ");
			printk(itoa(entry->page_size));
			printk(" ignored2: ");
			printk(itoa(entry->ignored2));
			printk(" pdp_address: ");
			printk(hex_to_str(entry->pdp_address));
			printk(" reserved: ");
			printk(itoa(entry->reserved));
			printk(" ignored3: ");
			printk(itoa(entry->ignored3));
			printk(" no_exec: ");
			printk(itoa(entry->no_exec));
			printk("\n");
		}
		void print_pdp_entry(pdp_entry_t* entry) {
			logk("\tPDP entry: ", KERNEL);
			printk("present: ");
			printk(itoa(entry->present));
			printk(" rw: ");
			printk(itoa(entry->rw));
			printk(" user: ");
			printk(itoa(entry->user));
			printk(" write_through: ");
			printk(itoa(entry->write_through));
			printk(" cache_disabled: ");
			printk(itoa(entry->cache_disabled));
			printk(" accessed: ");
			printk(itoa(entry->accessed));
			printk(" ignored1: ");
			printk(itoa(entry->ignored1));
			printk(" reserved1: ");
			printk(itoa(entry->reserved1));
			printk(" ignored2: ");
			printk(itoa(entry->ignored2));
			printk(" pd_address: ");
			printk(hex_to_str(entry->pd_address));
			printk(" reserved2: ");
			printk(itoa(entry->reserved2));
			printk(" ignored3: ");
			printk(itoa(entry->ignored3));
			printk(" no_exec: ");
			printk(itoa(entry->no_exec));
			printk("\n");
		}

		void print_pd_entry(pd_entry_t* entry) {
		}

		void print_pt_entry(pt_entry_t* entry) {
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
			//index_used_frames();
			for (int idx = 0 ; idx<512; idx++) {
				pml4_entry_t* i = &(pml4_dir->dir[idx+1]);
				if (i->present != 0) {
					print_pml4_entry(i);
					pdp_entry_t* p = (pdp_entry_t*) TO_VIRT_ADDR(i->pdp_address);
					print_pdp_entry(p+idx);
				}
			}

			//Allocate the first page and set the slab_head
			//slab_head = create_new_page();
			slab_head = create_multi_new_pages(1);
		}
	}
}
