#include<cstdint>
#include<cstddef>
#include<stdint.h>
#include<stddef.h>
#include<limine.h>
#include<stdlib.h>
#include<kernel.h>
#include<memory.h>
#include<gdt.h>
#include<interrupts.h>

// DO NOT REMOVE
extern "C" {
    int __cxa_atexit(void (*)(void *), void *, void *) { return 0; }
    void __cxa_pure_virtual() { halt(); }
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

	// Run our global constructors
	for (std::size_t i = 0; &__init_array[i] != __init_array_end; i++) {
	    __init_array[i]();
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

	Interrupts::init();
	logfk(KERNEL, "Interrupt initialization complete\n");

	logfk(KERNEL, "Gathering memory map\n");
	Memory::init_memmap();
	Memory::log_memory_info();
	logfk(KERNEL, "Initializing paging and memory management\n");
	Interrupts::test();
	Memory::Paging::init();
	Memory::Paging::test();
	logfk(KERNEL, "Paging and memory management intialization complete\n");


    	halt();
}
