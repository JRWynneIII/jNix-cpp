#include <cstdint>
#include <cstddef>
#include <limine.h>
#include <string.h>
#include <jnix.h>
#include <flanterm/flanterm.h>
#include <flanterm/backends/fb.h>

namespace {
	__attribute__((used, section(".limine_requests")))
	static volatile struct limine_framebuffer_request framebuffer_request = {
		.id = LIMINE_FRAMEBUFFER_REQUEST,
		.revision = 0
	};
}

namespace {
	__attribute__((used, section(".limine_requests_start")))
	static volatile LIMINE_REQUESTS_START_MARKER;

	__attribute__((used, section(".limine_requests_end")))
	static volatile LIMINE_REQUESTS_END_MARKER;
}


static void halt(void) {
	for (;;) {
	    asm("hlt");
	}
}

struct limine_framebuffer* framebuffer = nullptr;
struct flanterm_context* ft_ctx  = nullptr;

void init_framebuf() {
	if (framebuffer_request.response == nullptr
		|| framebuffer_request.response->framebuffer_count < 1) {
		   halt();
	}
	
	framebuffer = framebuffer_request.response->framebuffers[0];


	
	ft_ctx = flanterm_fb_init(
		NULL,
		NULL,
		(uint32_t*)framebuffer->address, framebuffer->width, framebuffer->height, framebuffer->pitch,
		framebuffer->red_mask_size, framebuffer->red_mask_shift,
		framebuffer->green_mask_size, framebuffer->green_mask_shift,
		framebuffer->blue_mask_size, framebuffer->blue_mask_shift,
		NULL,
		NULL, NULL,
		NULL, NULL,
		NULL, NULL,
		NULL, 0, 0, 1,
		0, 0,
		0
	);

}

void logk(char* msg, enum LOGLEVEL level) {
	switch(level) {
		case LOGLEVEL::KERNEL:
			printk("[KERNEL] ");
			printk(msg);
			break;
		case LOGLEVEL::USER:
			printk("[USER] ");
			printk(msg);
			break;
		case LOGLEVEL::INFO:
			printk("[INFO] ");
			printk(msg);
			break;
		case LOGLEVEL::ERROR:
			printk("[ERROR] ");
			printk(msg);
			break;
		default:
			printk(msg);
	}
}

void printk(char* msg) {
	//for (std::size_t i = 0; i < 100; i++) {
	//	volatile std::uint32_t *fb_ptr = static_cast<volatile std::uint32_t *>(framebuffer->address);
	//	fb_ptr[i * (framebuffer->pitch / 4) + i] = 0xffffff;
	//}
	flanterm_write(ft_ctx, msg, strlen(msg));

	return;
}
