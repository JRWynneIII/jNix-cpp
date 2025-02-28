#include<jnix.h>
#include<stdint.h>
#include<panic.h>
#include<limine.h>
#include<cstddef>
#include<stdlib.h>
#include<stdio.h>

extern uintptr_t endkernel;

#define KERNEL_BEGIN 0xffffffff80000000
#define KERNEL_END &endkernel
#define TOTAL_FRAMES 16384

enum FRAMESTATUS{
	FREE,
	SWAPPED,	// For future usage
	USED
}; 

namespace Memory {
	__attribute__((used, section(".limine_requests")))
	static volatile struct limine_memmap_request memmap = {
		.id = LIMINE_MEMMAP_REQUEST,
		.revision = 3
	};

	__attribute__((used, section(".limine_requests")))
	static volatile struct limine_hhdm_request hhdm = {
		.id = LIMINE_HHDM_REQUEST,
		.revision = 3
	};

	__attribute__((used, section(".limine_requests")))
	static volatile struct limine_kernel_address_request kernel_address = {
		.id = LIMINE_KERNEL_ADDRESS_REQUEST,
		.revision = 3
	};

	uint64_t total_mem_bytes = 0;
	uint64_t usable_mem_bytes = 0;
	uint64_t reserved_mem_bytes = 0;
	uint64_t bad_mem_bytes = 0;
	uint64_t reclaimable_mem_bytes = 0;
	uint64_t acpi_nvs_mem_bytes = 0;
	uint64_t unknown_mem_bytes = 0;
	uint64_t kernel_physical_addr_base;
	uint64_t kernel_virtual_addr_base;
	uint64_t virt_addr_offset;
	uint64_t hhdm_offset;
	// Should be a better way to do this
	// Should be able to use some kind of linked list, but ~~nO hEaP YeT~~
	mem_region usable_memory_regions[7];
	uint64_t region_idx = 0;

	void set_usable_mem_region(uint64_t idx, uint64_t base, uint64_t length) {
		mem_region region = {idx, base, length, nullptr};
		usable_memory_regions[region_idx] = region;
		region_idx++;
	}

	void log_memory_region(uint64_t idx, uint64_t base, uint64_t length) {
		logk("Found memory region ", KERNEL);
		printk(uitoa(idx));

		printk(": Base = ");
		printk(hex_to_str(base));
		printk(" [");
		printk(uitoa(base));
		printk("]");

		printk(", Length = ");
		printk(hex_to_str(length));
		printk(" [");
		printk(uitoa(length));
		printk("]");
		printk("\n");
	}
	

	void init_memmap() {
		uint64_t numentries = 0;
		struct limine_memmap_entry **entries;
		
		if (memmap.response != nullptr) {
			numentries = memmap.response->entry_count;
			entries = memmap.response->entries;

			for (uint64_t i = 0; i < numentries; i++) {
				total_mem_bytes += entries[i]->length;
				struct limine_memmap_entry *entry = entries[i];

				switch (entry->type) {
					case LIMINE_MEMMAP_USABLE: 
						usable_mem_bytes += entries[i]->length;
						set_usable_mem_region(i, entry->base, entry->length);
						break;
					case LIMINE_MEMMAP_ACPI_RECLAIMABLE: 
						reclaimable_mem_bytes += entries[i]->length;
						set_usable_mem_region(i, entry->base, entry->length);
						//log_memory_region(i, entry->base, entry->length);
						logk("Region ", KERNEL);
						printk(uitoa(i));
						printk(" is reclaimable.\n");
						break;
					case LIMINE_MEMMAP_BAD_MEMORY: 
						//TODO: Remove bad memory regions from usable_regions
						bad_mem_bytes += entries[i]->length;
						//log_memory_region(i, entry->base, entry->length);
						logk("Region ", KERNEL);
						printk(uitoa(i));
						printk(" is unusable; Bad memory!\n");
						break;
					case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
						reclaimable_mem_bytes += entries[i]->length;
						set_usable_mem_region(i, entry->base, entry->length);
						//log_memory_region(i, entry->base, entry->length);
						logk("Region ", KERNEL);
						printk(uitoa(i));
						printk(" is reclaimable.\n");
						break;
					case LIMINE_MEMMAP_RESERVED:
						reserved_mem_bytes += entries[i]->length;
						break;
					case LIMINE_MEMMAP_ACPI_NVS:
						acpi_nvs_mem_bytes += entries[i]->length;
						break;
					case LIMINE_MEMMAP_FRAMEBUFFER:
						break;
					case LIMINE_MEMMAP_KERNEL_AND_MODULES:
						break;
					default:
						unknown_mem_bytes += entries[i]->length;
						logk("Unknown entry type in limine memmap response!\n", ERROR); 
				}
			}
		} else {
			logk("Could not get memmap request from limine\n", ERROR);
		}

	}

	void log_memory_info() {
		for (auto cur : usable_memory_regions) {
			log_memory_region(cur.idx, cur.base, cur.length);
		}

		if (hhdm.response != nullptr) {
			hhdm_offset = hhdm.response->offset;
			logk("Higher Half Direct Map Offset : ", KERNEL);
			printk(hex_to_str(hhdm.response->offset));
			printk("\n");
		} else {
			logk("HHDM request to limine responded with NULL\n", ERROR);
		}

		if (kernel_address.response != nullptr) {
			kernel_physical_addr_base = kernel_address.response->physical_base;
			kernel_virtual_addr_base = kernel_address.response->virtual_base;
			virt_addr_offset = kernel_virtual_addr_base - kernel_physical_addr_base;
		} else {
			virt_addr_offset = 0;
			logk("Kernel address request to limine responded with NULL\n", ERROR);
		}

		logk("Kernel physical address: ", KERNEL);
		printk(hex_to_str(kernel_physical_addr_base));
		printk("\n");
		logk("Kernel virtual address: ", KERNEL);
		printk(hex_to_str(kernel_virtual_addr_base));
		printk("\n");
		logk("Kernel-land virtual memory offset: ", KERNEL);
		printk(hex_to_str(virt_addr_offset));
		printk("\n");
		// Print memory stats
		logk("Total memory: ", KERNEL);
		logk(uitoa(total_mem_bytes), NONE);
		logk("\n", NONE);
		logk("Usable memory: ", KERNEL);
		logk(uitoa(usable_mem_bytes), NONE);
		logk("\n", NONE);
		logk("Reserved memory: ", KERNEL);
		logk(uitoa(reserved_mem_bytes), NONE);
		logk("\n", NONE);
		logk("Bad memory: ", KERNEL);
		logk(uitoa(bad_mem_bytes), NONE);
		logk("\n", NONE);
		logk("Bootloader reclaimable memory: ", KERNEL);
		logk(uitoa(reclaimable_mem_bytes), NONE);
		logk("\n", NONE);
		logk("ACPI reclaimable memory: ", KERNEL);
		logk(uitoa(reclaimable_mem_bytes), NONE);
		logk("\n", NONE);
		logk("ACPI NVS memory: ", KERNEL);
		logk(uitoa(acpi_nvs_mem_bytes), NONE);
		logk("\n", NONE);
		logk("Unknown memory: ", KERNEL);
		logk(uitoa(unknown_mem_bytes), NONE);
		logk("\n", NONE);
	}
}

//TODO:
//	Get memmap from limine so we know where we actually are
//	Set up page directory
//	Create page table
//	disable and reenabled paging(?)
//
//Notes (incorrect?):
//	page table maps virtual -> physical addresses (frames)
//	page directory gives permissions for these maps
//	kernel sets up these permissions and sets page table entries for frames; keeps track of allocated frames and allocates more as needed
//	All processes have the same (or similar with a few bits toggled to denote page) virtual address space (or subset thereof)?
//	processor automatically uses the page directory to allow/deny access to phys mem regions
//	processor automatically uses the page table to map virtual to physical when addresses are accessed
//
//	when malloc-ing:
//		make sure we have enough free pages/frames in the pt
//			allocate more if needed (is this what sbrk does?)
//		if from kernel, can set supervisor bit? in pt or pd entry
//		mark n page frames as used (size = sizeof(requested type) * num objects; bytes; n = 1 or size/4096 bytes)
//		return virt address of  page.
//			does thi smean that a malloc of 1 int will take a full 4k page?
//	when freeing:
//		marks page frames as unused (if multiple non-adjacent page frames were allocated, and all we get back is a ptr, how do we know which frames to free?)
//			We use our in-kernel PageTable (not the page table tha tthe processor knows about) to keep track of all pages associated with the address
//		Invalidate page with `invlpg`
//
//
