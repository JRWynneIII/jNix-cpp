#include<cstddef>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<kernel.h>
#include<memory.h>

namespace Memory {
	void log_memory_region(uint64_t idx, uint64_t base, uint64_t length) {
		logfk(KERNEL, "Found memory region %d: Base=%x [%u], Length=%x [%u]\n",
				idx,
				base,
				base,
				length,
				length);
	}
	

	void log_memory_info() {
		for (auto cur : usable_memory_regions) {
			log_memory_region(cur.idx, cur.base, cur.length);
		}

		logfk(KERNEL, "Kernel physical address: %x\n", kernel_physical_addr_base);
		logfk(KERNEL, "Kernel virtual address: %x\n", kernel_virtual_addr_base);
		logfk(KERNEL, "Kernel-land virtual memory offset: %x\n", virt_addr_offset);
		//
		// Print memory stats
		logfk(KERNEL, "Total memory: %u\n", total_mem_bytes);
		logfk(KERNEL, "Usable memory: %u\n", usable_mem_bytes);
		logfk(KERNEL, "Reserved memory: %u\n",reserved_mem_bytes);
		logfk(KERNEL, "Bad memory: %u\n", bad_mem_bytes);
		logfk(KERNEL, "Bootloader reclaimable memory: %u\n", reclaimable_mem_bytes);
		logfk(KERNEL, "ACPI NVS memory: %u\n", acpi_nvs_mem_bytes);
		logfk(KERNEL, "Unknown memory: %u\n", unknown_mem_bytes);
	}

	namespace Paging {
		void dump_slab_list() {
			slab_t* cur = get_slab_head();
			int idx = 0;
			while (cur != get_slab_head() || idx == 0) {
				printfk("Slab #%d address: %x { %x, %x, %d, %d }\n", 
						idx, 
						cur, 
						cur->next, 
						cur->previous, 
						cur->size, 
						cur->is_free);
				cur = cur->next;
				idx++;
			}
		}

		void test() {
			logfk(KERNEL, "Running paging and allocation tests\n");
			uint64_t* t1 = kalloc(sizeof(uint64_t), 5);
			logfk(KERNEL, "Testing allocation of 40 bytes: %x ", t1);
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

			dump_slab_list();
			kfree(t1);
			logfk(KERNEL, "Free'd %x\n", t1);

			uint64_t* t3 = kalloc(sizeof(uint64_t), 512);
			logfk(KERNEL, "Testing allocation of 4096 bytes %x ", t3);
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

			dump_slab_list();
			kfree(t3);
			logfk(KERNEL, "Free'd %x\n", t3);

			uint64_t* t2 = kalloc(sizeof(uint64_t), 5);
			logfk(KERNEL, "Testing allocation of 40 bytes (second pass) %x ", t2);
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

			dump_slab_list();
			kfree(t2);
			logfk(KERNEL, "Free'd %x\n", t2);

			uint64_t* t4 = kalloc(sizeof(uint64_t), 512);
			logfk(KERNEL, "Testing allocation of 4096 bytes (second pass) %x ", t4);
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

			dump_slab_list();
			kfree(t4);
			logfk(KERNEL, "Free'd %x\n", t4);

			uint64_t* t5 = kalloc(sizeof(uint64_t), 1024);
			logfk(KERNEL, "Testing allocation of 8192 bytes %x ", t5);
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

			dump_slab_list();
			kfree(t5);
			logfk(KERNEL, "Free'd %x\n", t5);
			dump_slab_list();
			logfk(KERNEL, "Memory management testing complete\n");
		}
	}
}
