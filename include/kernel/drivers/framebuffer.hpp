#pragma once
#include <cstdint>
#include <flanterm/flanterm.h>
#include <limine.h>
#include <kernel/drivers/driver.hpp>
#include <kernel/devices/device_api.hpp>

typedef struct fbuffer {
	uint32_t* address;
    	uint64_t width;
    	uint64_t height;
    	uint64_t pitch;
    	uint8_t red_mask_size;
    	uint8_t red_mask_shift;
    	uint8_t green_mask_size;
    	uint8_t green_mask_shift;
    	uint8_t blue_mask_size;
    	uint8_t blue_mask_shift;
} framebuffer_t;

class framebuffer_driver : public driver_t {
private:
	bool enabled;
	framebuffer_t framebuffer;
	struct flanterm_context* ft_ctx;
public:
	framebuffer_driver();
	virtual void install();
	void late_init();
	virtual void irq_handler(struct registers* r) {};
	framebuffer_t get_framebuffer();
	void write(char* msg, uint64_t length);
};

namespace FrameBuffer {
	void init();
	void write(char*, uint64_t);
	framebuffer_driver& driver();
}
