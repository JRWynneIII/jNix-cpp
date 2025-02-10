#include <cstdint>
#include <cstddef>
#include <stdint.h>
#include <stddef.h>
#include <limine.h>
#include <jnix.h>

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
	logk("Loaded idt\n", KERNEL);
	init_idt();

    	halt();
}
