#pragma once
#include<cstdint>
#include<kernel/devices/device_api.hpp>

typedef struct tick_callback {
	uint64_t divisor;
	void (*callback)();
} tick_callback_t;

class pit_driver : public driver_t {
private:
	bool enabled;
	volatile uint64_t ticks;
	device_desc_t device_desc;
	vector<tick_callback_t>* callbacks;
public:
	pit_driver();
	void set_tick_hz(uint64_t hz);
	virtual void install();
	virtual void irq_handler(struct registers* r);
	uint64_t get_ticks();
	void on_tick(uint64_t divisior, void (*callback)());
	void run_callbacks();
};
