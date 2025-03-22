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
#include <kernel/streams.h>
#include<vector.hpp>
#include<string.hpp>
#include<kernel/interrupts.h>
#include<kernel/drivers/driver.hpp>
#include<kernel/drivers/framebuffer.hpp>

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
	framebuffer_driver& driver() {
		static framebuffer_driver fb = framebuffer_driver();
		return fb;
	}

	void init() {
		driver().install();
	}

	void write(char* msg, uint64_t length) {
		driver().write(msg, length);
	}
}

framebuffer_driver::framebuffer_driver() {
	this->enabled = false;
	this->framebuffer = {};
	this->ft_ctx = nullptr;
	this->desc = FRAMEBUFFER_DRIVER;
	this->set_name("framebuffer");
}

void framebuffer_driver::install() {
	if (!this->enabled) {
		if (framebuffer_request.response == nullptr
			|| framebuffer_request.response->framebuffer_count < 1) {
			   halt();
		}
		
		limine_framebuffer* lfb  = framebuffer_request.response->framebuffers[0];
		
		this->framebuffer = {
			(uint32_t*)lfb->address, 
			lfb->width, 
			lfb->height, 
			lfb->pitch,
			lfb->red_mask_size, 
			lfb->red_mask_shift,
			lfb->green_mask_size, 
			lfb->green_mask_shift,
			lfb->blue_mask_size, 
			lfb->blue_mask_shift
		};
		
		
		this->ft_ctx = flanterm_fb_init(
			NULL,
			NULL,
			framebuffer.address, framebuffer.width, framebuffer.height, framebuffer.pitch,
			framebuffer.red_mask_size, framebuffer.red_mask_shift,
			framebuffer.green_mask_size, framebuffer.green_mask_shift,
			framebuffer.blue_mask_size, framebuffer.blue_mask_shift,
			NULL,
			NULL, NULL,
			NULL, NULL,
			NULL, NULL,
			NULL, 0, 0, 1,
			0, 0,
			0
		);
	}
	this->enable();
}

void framebuffer_driver::write(char* msg, uint64_t length) {
	flanterm_write(ft_ctx, msg, length);
}



