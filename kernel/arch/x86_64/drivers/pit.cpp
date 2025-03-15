#include<cstdint>
#include<kernel/interrupts.h>
#include<kernel/drivers.h>
#include<kernel/devices/devicetree.h>
#include<kernel/driver/pit.hpp>
#include<kernel.h>
#include<kernel/port.h>
#include<kernel/devices/devices.hpp>

pit_driver::pit_driver() {
	this->enabled = false;
	this->ticks = 0;
	this->irq_no = 0;
	this->desc = TIMER_DRIVER;
	this->device_desc = TIMER;
	this->set_name("pit");
}

void pit_irq_wrapper(struct registers* r) {
	Drivers::get_timer_driver()->irq_handler(r);
}

void pit_driver::install() {
	this->set_tick_hz(1000); //1 tick == 1ms
	this->enabled = true;
	Interrupts::install_handler(this->irq_no, pit_irq_wrapper);
	//Add device to device tree
	Devices::add_device(this, this->device_desc, "Programmable Interrupt Timer");
	this->enable();
}

void pit_driver::set_tick_hz(uint64_t hz) {
	uint64_t div = 1193180/hz;
	outportb(0x43, 0x36);
	outportb(0x40, div & 0xFF);
	outportb(0x40, div >> 8);
}

void pit_driver::irq_handler(struct registers* r) {
	this->ticks++;
}

uint64_t pit_driver::get_ticks() {
	return this->ticks;
}
