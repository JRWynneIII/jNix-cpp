#include <cstdint>
#include <cstddef>
#include <limine.h>
#include <string.h>
#include <kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstdarg>
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


namespace FrameBuffer {
	struct limine_framebuffer* framebuffer = nullptr;
	struct flanterm_context* ft_ctx  = nullptr;
	
	void init() {
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

	void write(char* msg, uint64_t length) {
		flanterm_write(ft_ctx, msg, length);
	}
}
	

void logk(char* msg, enum LOGLEVEL level) {
	if (msg != nullptr) {
		switch(level) {
			case LOGLEVEL::KERNEL:
				printfk("[KERNEL] %s", msg);
				break;
			case LOGLEVEL::USER:
				printfk("[USER] %s", msg);
				break;
			case LOGLEVEL::INFO:
				printfk("[INFO] %s", msg);
				break;
			case LOGLEVEL::ERROR:
				printfk("[ERROR] %s", msg);
				break;
			case LOGLEVEL::PANIC:
				printfk("[PANIC] %s", msg);
				break;
			default:
				printk(msg);
		}
	} else {
		printk("Error in logk: msg is null!");
	}

}


void printfk(char* format...) {
	va_list args;
	va_start(args, format);

	char* cur = format;
	uint64_t idx = 0;
	while(*cur != '\0') {
		if (*cur == '%') {
			cur++;
			if (*cur == 'd') {
				int64_t arg = va_arg(args, int64_t);
				printk(itoa(arg));
			} else if (*cur == 'u') {
				uint64_t arg = va_arg(args, uint64_t);
				printk(uitoa(arg));
			} else if (*cur == 'x') {
				uint64_t arg = va_arg(args, uint64_t);
				printk(hex_to_str(arg));
			} else if (*cur == 's') {
				char* arg = va_arg(args, char*);
				printk(arg);
			} else if (*cur == '%') {
				putchk("%");
			} else {
				putchk("%");
				putchk(*cur);
			}
		} else {
			putchk(*cur);
		}
		cur++;
	}
	va_end(args);
}

void putchk(char c) {
	FrameBuffer::write(&c, 1);
}

void printk(char* msg) {
	if (msg != nullptr) {
		FrameBuffer::write(msg, strlen(msg));
	} else {
		char* m = "Error in printk: msg is null!";
		FrameBuffer::write(m, strlen(m));
	}
	return;
}
