#include<cstddef>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<kernel.h>
#include<kernel/memory.h>
#include<kernel/memory/addrspace.hpp>
#include<kernel/ptr.hpp>
#include<cstdint>

//These are basically convenience functions around kernel_address_space and other addrspace_t objs
namespace Memory {
	namespace VMM {
		addrspace_t& kernel_address_space() {
			static addrspace_t as = addrspace_t(Memory::hhdm_offset, false);
			return as;
		}

		uintptr_t alloc_contiguous_pages(uint64_t num, bool rw, bool noexec, bool isuser, addrspace_t* as) {
			uintptr_t frames = Memory::PMM::alloc_contiguous_frames(num);
			//printfk("VMM: Got %d frames from PMM at %x\n", num, frames);
			uintptr_t frame_end = frames + (4096 * num);
			for (uintptr_t cur = frames; cur < frame_end; cur += 4096) {
				//printfk("Mapping page %x to %x\n", cur, TO_VIRT_ADDR(cur));
				as->map_page(TO_VIRT_ADDR(cur), cur, rw, noexec, false, false, isuser);
				//printfk("Mapped page %x to %x\n", cur, TO_VIRT_ADDR(cur));
			}
			return TO_VIRT_ADDR(frames);
		}

		void modify_page_perms(uintptr_t virt, bool rw, bool noexec, bool isuser, addrspace_t* as) {
			pt_entry_t* page = as->lookup(virt);
			page->rw = rw;
			page->no_exec = noexec;
			page->user = isuser;
		}

		void modify_page_perms(uintptr_t virt, bool rw, bool noexec, bool isuser) {
			addrspace_t& as = kernel_address_space();
			modify_page_perms(virt, rw, noexec, isuser, &as);
		}

		uintptr_t alloc_contiguous_pages(uint64_t num, bool rw, bool noexec, bool isuser) {
			addrspace_t& as = kernel_address_space();
			alloc_contiguous_pages(num, rw, noexec, isuser, &as);
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

			kernel_address_space().bootstrap();
			kernel_address_space().activate();
		}
	}
}
