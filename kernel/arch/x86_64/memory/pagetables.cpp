#include<stdint.h>
#include<panic.h>
#include<limine.h>
#include<cstddef>
#include<stdlib.h>
#include<stdio.h>

namespace Memory {
	namespace Tables {
		typedef uintptr_t pd_entry_t;
		typedef uintptr_t pud_entry_t;
		typedef uintptr_t pmd_entry_t;
		typedef uintptr_t pt_entry_t;

		pd_entry_t page_directory[512];
		pud_entry_t page_upper_directory[512];
		pmd_entry_t page_middle_directory[512];
		pt_entry_t page_table[512];
		//TODO: this type might be too....big? Each index is only 12bit so...
		//Virtual address to page table index
		uintptr_t virt_to_pt_offset(uintptr_t virt) {
			return (virt >> 12) & 0x1FF;
		}

		//Virtual address to page middle directory index
		uintptr_t virt_to_pmd_offset(uintptr_t virt) {
			return (virt >> 21) & 0x1FF;
		}

		//Virtual address to page upper directory index
		uintptr_t virt_to_pud_offset(uintptr_t virt) {
			return (virt >> 30) & 0x1FF;
		}

		//Virtual address to page global directory index
		uintptr_t virt_to_pgd_offset(uintptr_t virt) {
			return (virt >> 39) & 0x1FF;
		}


		// This is for checking the last bit to see if it's set to true, which is present
		// NOTE: May be able to be used for more than just the PGD entry
		bool entry_is_present(uintptr_t entry) {
			return (entry & 0b0001) == 0x1;
		}

		// Is r/w bit set
		bool entry_is_writeable(uintptr_t entry) {
			return (entry & 0b0010) == 0x2;
		}

		// Is the user bit set
		bool entry_is_user_accessible(uintptr_t entry) {
			return (entry & 0b0100) == 0x4;
		}
		
		// Is the entry write through bit set
		bool entry_is_write_through(uintptr_t entry) {
			return (entry & 0b1000) == 0x8;
		}

		// Is the page cache bit set
		bool entry_is_cache_disabled(uintptr_t entry) {
			return (entry & 0b10000) == 0x10;
		}

		// Is the accessed bit set
		bool entry_is_accessed(uintptr_t entry) {
			return (entry & 0b100000) == 0x20;
		}

		// Is the dirty bit set
		bool entry_is_dirty(uintptr_t entry) {
			return (entry & 0b1000000) == 0x40;
		}
		
		// Is the executable bit set
		bool entry_is_executable(uintptr_t entry) {
			return (entry & (1ull << 63)) == 0x0;
		}
		
		// Is the page size bit set
		// Means this is the 'last level entry' and contains the physical address?
		// Means this is a huge page
		// If in PUD, it is 1G
		// If in PMD, it is 2M
		// TODO: rewrite these entry_* functions to work with hugepages
		bool entry_is_page_size(uintptr_t entry) {
			return (entry & 0b010000000) == 0x80;
		}

		// Is the page size bit set
		bool entry_is_PAT(uintptr_t entry) {
			return (entry & 0b010000000) == 0x80;
		}

		// Is the global bit set
		bool entry_is_global(uintptr_t entry) {
			return (entry & 0b0100000000) == 0x100;
		}

		// Decodes the physical address from the page table entry bits 11-50
		uintptr_t get_phys_addr_from_pt_entry(uintptr_t entry) {
			//TODO: Might have to add the 'last 12 bits from the virt address' to this addr
			return entry & ~((1ull << 11) - 1) & ((1ull << 50) -1);
		}

		// Get bits 12-51 of the pgd entry for the PUD address
		uintptr_t get_next_directory_address(uintptr_t entry) {
			return entry & ~((1ull << 12) - 1) & ((1ull << 51) -1);
		}
	}
}
