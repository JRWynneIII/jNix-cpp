#pragma once
#include<cstdint>
#include<kernel/devicetree.h>

class pit_driver : public driver_t {
private:
	bool enabled;
	volatile uint64_t ticks;
	device_desc_t device_desc;
public:
	pit_driver();
	void set_tick_hz(uint64_t hz);
	virtual void install();
	virtual void irq_handler(struct registers* r);
	uint64_t get_ticks();
};
