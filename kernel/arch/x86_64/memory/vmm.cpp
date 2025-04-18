#include<cstddef>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<kernel.h>
#include<kernel/memory.h>
#include<kernel/memory/addrspace.hpp>
#include<kernel/ptr.hpp>
#include<cstdint>

namespace Memory {
	namespace VMM {
		addrspace_t& kernel_address_space() {
			static addrspace_t as = addrspace_t(Memory::hhdm_offset);
			return as;
		}

		//Convenience wrapper for create_page_table_entry so that you don't have to build your own frame object
		void map_address(uintptr_t virt_addr, uintptr_t phys_addr, bool readonly, bool executable, bool isuser, addrspace_t* as) {
			as->map_page(virt_addr, phys_addr, !readonly, !executable, false, false, isuser);
		}

		void create_page_table_entry(uintptr_t phys, uintptr_t virt, bool rw, bool noexec, bool isuser, addrspace_t* as) {
			map_address(phys, virt, rw, noexec, isuser, as);
		}

		void create_page_table_entry(frame_t* frame, addrspace_t* as) {
			map_address(frame->phys_addr, frame->virt_addr, !frame->is_readonly, !frame->is_executable, frame->is_user, as);
		}

		pt_entry_t* lookup_address(uintptr_t virt_addr) {
			return kernel_address_space().lookup(virt_addr);
		}

		void init() {
			logfk(KERNEL, "Initing new addr space\n");
			// The page directory address is stored in CR3; This is a unique register per *thread*
			// NOTE: the above when implementing SMT
			uintptr_t pml4_ptr;
			asm volatile("mov %%cr3, %0" : "=r"(pml4_ptr));
			// CR3 holds more than just the pgd address, so we mask off what we don't want
			pml4_ptr &= ~0xFFF; 
			//pml4_ptr = TO_VIRT_ADDR(pml4_ptr);
			pml4_dir_t* pml4 = (pml4_dir_t*) TO_VIRT_ADDR(pml4_ptr);

			kernel_address_space().bootstrap(pml4);
		}
	}
}
