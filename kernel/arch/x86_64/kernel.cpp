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
#include<kernel/drivers.h>
#include<cxxabi.h>
#include<kernel/devicetree.h>

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


// The following will be our kernel's entry point.
extern "C" void _start(void) {
	if (LIMINE_BASE_REVISION_SUPPORTED == false) {
		halt();
	}

	FrameBuffer::init();
	printk("Booting jnix....\n\n");
	logfk(KERNEL, "Initialized framebuffer\n");
	if (boot_time_req.response == nullptr) {
		logfk(ERROR, "Could not get boot time from Limine/RTC");
	}

	int64_t boottime = boot_time_req.response->boot_time;
	logfk(KERNEL, "Boot time: %d\n", boottime);

	logfk(KERNEL, "Loading new GDT\n");
	GDT::init();
	logfk(KERNEL, "GDT initialization complete\n");


	logfk(KERNEL, "Gathering memory map\n");
	Memory::init_memmap();
	Memory::log_memory_info();
	logfk(KERNEL, "Initializing paging and memory management\n");
//	Interrupts::test();
	Memory::Paging::init();
	logfk(KERNEL, "Paging and memory management intialization complete\n");
	
	// Run our global constructors
	for (std::size_t i = 0; &__init_array[i] != __init_array_end; i++) {
	    __init_array[i]();
	}

	logfk(KERNEL, "Loading drivers...\n");
	Drivers::load_drivers();
	logfk(KERNEL, "Initializing drivers...\n");
	Interrupts::init();
	logfk(KERNEL, "Interrupt initialization complete\n");
	Drivers::init();
	Devices::dump_device_tree();

	for(;;) {
		char c = getch();
		printfk("%c",c);
		if (c == '\n') {
			Device* timer = Devices::get_device_by_path("pit.programmable_interrupt_timer.1");
			if (timer != nullptr) {
				pit_driver* driver = static_cast<pit_driver*>(timer->get_driver());
				printfk("Cur ticks: %d\n", driver->get_ticks());
			} else {
				logfk(ERROR, "Could not find timer\n");
			}
		}
	}

    	halt();
}
