#include<cstddef>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<kernel.h>
#include<memory.h>

namespace Memory {
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
	

	void log_memory_info() {
		for (auto cur : usable_memory_regions) {
			log_memory_region(cur.idx, cur.base, cur.length);
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

	namespace Paging {
		void log_slab_entry(slab_t* slab) {
			printk(" address: ");
			printk(hex_to_str(slab));
			printk(" ");
			printk("{ ");
			printk(hex_to_str(slab->next));
			printk(", ");
			printk(hex_to_str(slab->previous));
			printk(", ");
			printk(itoa(slab->size));
			printk(", ");
			printk(itoa(slab->is_free));
			printk("}\n");
		}

		void dump_slab_list() {
			slab_t* cur = get_slab_head();
			int idx = 0;
			while (cur != get_slab_head() || idx == 0) {
				printk("Slab #");
				printk(itoa(idx));
				log_slab_entry(cur);
				cur = cur->next;
				idx++;
			}
		}

		void test() {
			logk("Running paging and allocation tests\n", KERNEL);
			logk("Testing allocation of 40 bytes ", KERNEL);
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

			dump_slab_list();
			kfree(t1);
			logk("Free'd t1\n", KERNEL);

			logk("Testing allocation of 4096 bytes ", KERNEL);
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

			dump_slab_list();
			kfree(t3);
			logk("Free'd t3\n", KERNEL);

			logk("Testing allocation of 40 bytes (second pass) ", KERNEL);
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

			dump_slab_list();
			kfree(t2);
			logk("Free'd t2\n", KERNEL);

			logk("Testing allocation of 4096 bytes (second pass) ", KERNEL);
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

			dump_slab_list();
			kfree(t4);
			logk("Free'd t4\n", KERNEL);

			logk("Testing allocation of 8192 bytes ", KERNEL);
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

			dump_slab_list();
			kfree(t5);
			logk("Free'd t5\n", KERNEL);
			dump_slab_list();
			logk("Memory management testing complete\n", KERNEL);
		}
	}
}
