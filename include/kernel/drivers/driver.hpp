#pragma once
#include<kernel.h>
//#include<cstdint>
//#include<kernel/interrupts.h>
//#include<kernel/ptr.hpp>

typedef enum driver_desc {
	PS2_DRIVER,
	TIMER_DRIVER,
	FRAMEBUFFER_DRIVER,
	RTC_DRIVER
} driver_desc_t;

class driver_t {
private:
	char* name;
	uint64_t attached_devices;
public:
	bool enabled;
	driver_desc_t desc;
	uint64_t irq_no;

	driver_t() {
		this->irq_no = 999;
		this->attached_devices = 0;
		this->enabled = false;
	}

	void disable() {
		this->enabled = false;
		Interrupts::mask_irq(this->irq_no);
	}

	void enable() {
		this->enabled = true;
		Interrupts::unmask_irq(this->irq_no);
	}

	void disable(uint64_t irq_no) {
		this->enabled = false;
		Interrupts::mask_irq(irq_no);
	}


	void enable(uint64_t irq_no) {
		this->enabled = true;
		Interrupts::unmask_irq(irq_no);
	}

	virtual void install() = 0;
	virtual void irq_handler(struct registers* r) = 0;

	void add_device() {
		this->attached_devices++;
	}

	void rm_device() {
		this->attached_devices--;
	}

	uint64_t num_devices() {
		return this->attached_devices;
	}

	char* get_name() {
		return this->name;
	}

	void set_name(char* name) {
		this->name = name;
	}
};
