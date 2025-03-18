#include<cstdint>
#include<cstddef>
#include<stdint.h>
#include<stddef.h>
#include<limine.h>
#include<stdlib.h>
#include<kernel.h>
#include<kernel/memory.h>
#include<kernel/gdt.h>
#include<kernel/interrupts.h>
#include<kernel/drivers/driver_api.hpp>
#include<cxxabi.h>
#include<kernel/devices/device_api.hpp>
#include<kernel/monitor.hpp>

// DO NOT REMOVE
extern "C" {
	int __cxa_atexit(void (*)(void *), void *, void *) { return 0; }
	void __cxa_pure_virtual() {
		logfk(ERROR, "Missing virtual function\n");
		halt(); 
	}

	void *__dso_handle;
}

// Extern declarations for global constructors array.
extern void (*__init_array[])();
extern void (*__init_array_end[])();

namespace {
	__attribute__((used, section(".limine_requests")))
	static volatile LIMINE_BASE_REVISION(3);
}

namespace {
	__attribute__((used, section(".limine_requests")))
	static volatile struct limine_boot_time_request boot_time_req = {
		.id = LIMINE_BOOT_TIME_REQUEST,
		.revision = 0
	};
}

//TODO: Fix this such that we have a large enough stack here
//namespace {
//	__attribute__((used, section(".limine_requests")))
//	static volatile struct limine_stack_size_request stack_req = {
//		.id = LIMINE_STACK_SIZE_REQUEST,
//		.revision = 0,
//		.stack_size = 4096
//	};
//}


// The following will be our kernel's entry point.
extern "C" void kmain(void) {
	if (LIMINE_BASE_REVISION_SUPPORTED == false) {
		halt();
	}

	FrameBuffer::init();
	printk("Booting jnix....\n\n");
	logfk(KERNEL, "Initialized framebuffer\n");
	if (boot_time_req.response == nullptr) {
		logfk(ERROR, "Could not get boot time from Limine/RTC");
	}

	logfk(KERNEL, "Loading new GDT\n");
	GDT::init();
	logfk(KERNEL, "GDT initialization complete\n");


	logfk(KERNEL, "Gathering memory map\n");
	Memory::init_memmap();
	Memory::log_memory_info();
	logfk(KERNEL, "Initializing paging and memory management\n");
//	Interrupts::test();
	Memory::Paging::init();
	Kernel::Log(KERNEL, "Paging and memory management intialization complete\n");
	
	// Run our global constructors
	for (std::size_t i = 0; &__init_array[i] != __init_array_end; i++) {
	    __init_array[i]();
	}

	Kernel::Log(KERNEL, "Loading drivers...\n");
	Drivers::load_drivers();
	Kernel::Log(KERNEL, "Initializing drivers...\n");
	Interrupts::init();
	Kernel::Log(KERNEL, "Interrupt initialization complete\n");
	Drivers::init();
	Devices::dump_device_tree();
	int64_t boottime = boot_time_req.response->boot_time;
	Kernel::Log(KERNEL, "Boot time: %d\n", boottime);
	//Memory::Paging::dump_slab_list();
	//Memory::Paging::test();

	Kernel::Log(KERNEL, "Starting kernel-space monitor\n\n\n\n\n\n");
	printfk("Welcome to jnix!\n");
	Monitor::start();


    	halt();
}
