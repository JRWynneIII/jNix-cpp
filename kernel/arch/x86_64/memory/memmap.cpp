#include<kernel.h>
#include<stdint.h>
#include<panic.h>
#include<limine.h>
#include<cstddef>
#include<stdlib.h>
#include<stdio.h>
#include<memory.h>

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
						logfk(KERNEL, "Region %d is reclaimable\n", i);
						break;
					case LIMINE_MEMMAP_BAD_MEMORY: 
						//TODO: Remove bad memory regions from usable_regions
						bad_mem_bytes += entries[i]->length;
						//log_memory_region(i, entry->base, entry->length);
						logfk(KERNEL, "Region %d is unusable; Bad memory!\n", i);
						break;
					case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
						reclaimable_mem_bytes += entries[i]->length;
						set_usable_mem_region(i, entry->base, entry->length);
						logfk(KERNEL, "Region %d is reclaimable\n", i);
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
						logfk(ERROR, "Unknown entry type in limine memmap response!\n"); 
				}
			}
		} else {
			logfk(ERROR, "Could not get memmap request from limine\n");
		}

		if (hhdm.response != nullptr) {
			hhdm_offset = hhdm.response->offset;
			logfk(KERNEL, "Kernel virtual memory offset: %x\n", hhdm.response->offset);
		} else {
			logfk(ERROR, "HHDM request to limine responded with NULL\n");
		}

		if (kernel_address.response != nullptr) {
			kernel_physical_addr_base = kernel_address.response->physical_base;
			kernel_virtual_addr_base = kernel_address.response->virtual_base;
			virt_addr_offset = kernel_virtual_addr_base - kernel_physical_addr_base;
		} else {
			virt_addr_offset = 0;
			logfk(ERROR, "Kernel address request to limine responded with NULL\n");
		}

	}
}
