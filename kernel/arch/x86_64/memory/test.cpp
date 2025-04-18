#include<cstddef>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<kernel.h>
#include<kernel/memory.h>
#include<kernel/ptr.hpp>
#include<unwind.h>
#include<kernel/memory.h>

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

	namespace Allocation {
		void dump_slab_list() {
			slab_t* cur = get_slab_head();
			int idx = 0;
			while (cur != get_slab_head() || idx == 0) {
				printfk("Slab #%d address: %x { %x, %x, %u, %d, %d, %d }\n", 
						idx, 
						cur, 
						cur->next, 
						cur->previous, 
						cur->size, 
						cur->is_free,
						cur->is_readonly,
						cur->is_executable);
				cur = cur->next;
				idx++;
			}
		}

		// Function to test the C++ operators new/delete
		void test_operators() {
			logfk(KERNEL, "Running paging and allocation tests\n");
			uint64_t* t1 = new uint64_t[5]; //kalloc(sizeof(uint64_t), 5);
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

			//dump_slab_list();
			delete t1;
			logfk(KERNEL, "Free'd %x\n", t1);
			//dump_slab_list();
			logfk(KERNEL, "Memory management testing complete\n");
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

			kfree(t1);
			logfk(KERNEL, "Free'd %x\n", t1);

			uint32_t* t3 = kalloc(sizeof(uint32_t), 127);
			logfk(KERNEL, "Testing allocation of 4096 bytes %x ", t3);
			for (int i = 0; i< 127; i++) {
				t3[i] = i+1;
			}
			pass = true;
			for (int i=0; i<127; i++) {
				if (t3[i] != i+1) {
					pass = false;
				}
			}

			if (pass) {
				printk(" PASSED\n");
			} else {
				printk(" FAILED\n");
			}

			kfree(t3);
			logfk(KERNEL, "Free'd %x\n", t3);

			//uint64_t* t2 = kalloc(sizeof(uint64_t), 5);
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

			kfree(t2);
			logfk(KERNEL, "Free'd %x\n", t2);

			uint32_t* t4 = kalloc(sizeof(uint32_t), 127);
			logfk(KERNEL, "Testing allocation of 4096 bytes (second pass) %x ", t4);
			for (int i = 0; i< 127; i++) {
				t4[i] = i+1;
			}
			pass = true;
			for (int i=0; i<127; i++) {
				if (t4[i] != i+1) pass = false;
			}

			if (pass) {
				printk(" PASSED\n");
			} else {
				printk(" FAILED\n");
			}

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

			kfree(t5);
			logfk(KERNEL, "Free'd %x\n", t5);
			logfk(KERNEL, "Memory management testing complete\n");
		}

		void ptr_t_test() {
			// Test manual allocation of pointers. (NOTE: Kmalloc will return a ptr_t)
			printfk("Allocating t1\n");
			ptr_t<uint64_t> t1 = kmalloc<uint64_t>(sizeof(uint64_t) * 5);
			// Wrap a raw pointer in a ptr_t
			printfk("Allocating t2\n");
			ptr_t<uint64_t> t2 = ptr_t<uint64_t>(new uint64_t[5], sizeof(uint64_t) * 5);
			// Implicit kmalloc in constructor
			// NOTE: These should be the default ways to allocate memory in the kernel!
			printfk("Allocating t3\n");
			ptr_t<uint64_t> t3 = ptr_t<uint64_t>(sizeof(uint64_t) * 5);
			printfk("Allocating t4\n");
			//We *probably* don't want to do this too much, but it does work.
			//This functionally creates a uint64_t**, since t4 is a ptr that contains a ptr
			ptr_t<uint64_t>* t4 = new ptr_t<uint64_t>(sizeof(uint64_t) * 5);


			// Set values
			for (int i = 0; i < 5; i++) t1[i] = i*2;
			for (int i = 0; i < 5; i++) t2[i] = i*3;
			for (int i = 0; i < 5; i++) t3[i] = i*4;
			for (int i = 0; i < 5; i++) t4->at(i) = i*5;

			//Test triggering the failure case
			//t1[6] = 999;
			//t1[7] = 888;

			//Dump contents to screen
			for (int i = 0; i < 5; i++) printfk("%d\t", t1[i]);
			printk("\n");
			for (int i = 0; i < 5; i++) printfk("%d\t", t2[i]);
			printk("\n");
			for (int i = 0; i < 5; i++) printfk("%d\t", t3[i]);
			printk("\n");
			for (int i = 0; i < 5; i++) printfk("%d\t", t4->at(i));
			printk("\n");

			// Dump the ptr_t values to the screen
			logfk(INFO, "%x: { ptr = %x, size = %d }\n", &t1, t1.get_raw(), t1.get_size(), t1.get_num_elements());
			logfk(INFO, "%x: { ptr = %x, size = %d }\n", &t2, t2.get_raw(), t2.get_size(), t2.get_num_elements());
			logfk(INFO, "%x: { ptr = %x, size = %d }\n", &t3, t3.get_raw(), t3.get_size(), t3.get_num_elements());
			logfk(INFO, "%x: { ptr = %x, size = %d }\n", t4, t4->get_raw(), t4->get_size(), t4->get_num_elements());
			
			// Need to manually delete this since it was manually allocated
			// This will trigger the destructor of the object, deallocating the internal pointer
			delete t4;
		}
	}
}
