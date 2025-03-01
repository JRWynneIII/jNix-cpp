#include <cstdint>
#include <cstddef>
#include <stdint.h>
#include <stddef.h>
#include <limine.h>
#include <jnix.h>
#include <stdlib.h>

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent.

static void halt(void) {
    for (;;) {
        asm("hlt");
    }
}

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

	// Run out global constructors
	for (std::size_t i = 0; &__init_array[i] != __init_array_end; i++) {
	    __init_array[i]();
	}

	init_framebuf();
	printk("Booting jnix....\n\n");
	logk("Initialized framebuffer\n", KERNEL);
	if (boot_time_req.response == nullptr) {
		logk("Could not get boot time from Limine/RTC", ERROR);
		halt();
	}

	int64_t boottime = boot_time_req.response->boot_time;
	logk("Boot time: ", KERNEL);
	logk(itoa((int)boottime), NONE);
	logk("\n", NONE);
	logk("Loading new GDT", KERNEL);
	GDT::init();
	logk("....DONE\n", NONE);
	Interrupts::init();
	logk("Interrupt initialization complete\n", KERNEL);

	logk("Gathering memory info\n", KERNEL);
	Memory::init_memmap();
	Memory::log_memory_info();
	logk("Initializing page directory\n", KERNEL);
	logk("Initializing page table\n", KERNEL);
	//Interrupts::test();
	Memory::Paging::init();
	Memory::Paging::test();


    	halt();
}
