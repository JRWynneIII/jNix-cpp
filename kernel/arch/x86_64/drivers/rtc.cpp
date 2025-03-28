#include<cstdint>
#include<kernel.h>
#include<kernel/port.h>
#include<kernel/interrupts.h>
#include<kernel/drivers/driver_api.hpp>
#include<kernel/drivers/rtc.hpp>

rtc_driver::rtc_driver() {
	this->hours = 0;
	this->min = 0;
	this->sec = 0;
	this->day = 0;
	this->month = 0;
	this->year = 0;
	this->irq_no = 8;
	this->desc = RTC_DRIVER;
	this->device_desc = CLOCK;
	this->set_name("rtc");
}

void rtc_driver::enable_interrupt() {
	__asm__ __volatile__("cli");
	Interrupts::disable_nmi();
	// Set the interrupt tick rate to the slowest, since we only are using the interrupt to set the time
	uint8_t tickrate = 15 & 0x0F;
	uint8_t regA = this->read_regA();
	this->write_regA((regA & 0xF0) | tickrate);

	//Enable the interrupt
	uint8_t regB = this->read_regB();
	regB |= 0x40; //Set the interrupt enable bit
	this->write_regB(regB);
	printfk("RTC RegC: %x\n", this->read_regC());
	Interrupts::enable_nmi();
	__asm__ __volatile__("sti");
}

void update_time_cb() {
	rtc_driver* rtc = Drivers::get_rtc_driver();
	rtc->update_time();
}

void rtc_driver::install() {
	logfk(KERNEL, "Installing RTC Driver\n");
	//Interrupt will fire immediately! So ensure its set in the IDT first
	//before enabling the rtc interrupt
	//Interrupts::install_handler(this->irq_no, rtc_handler_wrapper);
	//this->enable_interrupt();
	
	this->enable();
	Devices::add_device(this, this->device_desc, "Real Time Clock");
	Device* pit = Devices::get_device_by_path("pit.programmable_interrupt_timer.1");
	if (pit != nullptr) {
		logfk(KERNEL, "Installing time update on tick 1000\n");
		pit_driver* d = static_cast<pit_driver*>(pit->get_driver());
		//TODO: This might cause latency with other interrupts. Figure out how to do this asyncronously
		d->on_tick(100, update_time_cb);
	}
}

bool rtc_driver::is_ready_to_read_time() {
	uint8_t regA = this->read_regA();
	if ((regA & 0x80) == 0) return true;
	return false;
}

void rtc_driver::wait_to_read_time() {
	//Wait until bit is set, then wait until bit is unset
	while((this->is_ready_to_read_time())) {}
	while(!(this->is_ready_to_read_time())) {}
}

uint8_t rtc_driver::read_reg0() {
	outportb(0x70, 0x80); // Disables NMI and and selects reg A for next r/w
	return inportb(0x71); //Read from reg
}

uint8_t rtc_driver::read_reg2() {
	outportb(0x70, 0x82); // Disables NMI and and selects reg B for next r/w
	return inportb(0x71); //Read from reg
}

uint8_t rtc_driver::read_reg4() {
	outportb(0x70, 0x84); // Disables NMI and and selects reg C for next r/w
	return inportb(0x71); //Read from reg 
}

uint8_t rtc_driver::read_reg7() {
	outportb(0x70, 0x87); // Disables NMI and and selects reg C for next r/w
	return inportb(0x71); //Read from reg 
}

uint8_t rtc_driver::read_reg8() {
	outportb(0x70, 0x88); // Disables NMI and and selects reg C for next r/w
	return inportb(0x71); //Read from reg 
}

uint8_t rtc_driver::read_reg9() {
	outportb(0x70, 0x89); // Disables NMI and and selects reg C for next r/w
	return inportb(0x71); //Read from reg 
}

uint8_t rtc_driver::read_regA() {
	outportb(0x70, 0x8A); // Disables NMI and and selects reg A for next r/w
	return inportb(0x71); //Read from reg
}

uint8_t rtc_driver::read_regB() {
	outportb(0x70, 0x8B); // Disables NMI and and selects reg B for next r/w
	return inportb(0x71); //Read from reg
}

uint8_t rtc_driver::read_regC() {
	outportb(0x70, 0x8C); // Disables NMI and and selects reg C for next r/w
	return inportb(0x71); //Read from reg 
}

void rtc_driver::write_regA(uint8_t byte) {
	uint8_t cur = inportb(0x70);
	outportb(0x70, cur | 0x0A); //Set next write for reg A
	outportb(0x71, byte); 
}

void rtc_driver::write_regB(uint8_t byte) {
	uint8_t cur = inportb(0x70);
	outportb(0x70, cur | 0x0B); //Set next write for reg B
	outportb(0x71, byte); 
}

void rtc_driver::write_regC(uint8_t byte) {
	uint8_t cur = inportb(0x70);
	outportb(0x70, cur | 0x0C); //Set next write for reg C
	outportb(0x71, byte); 
}

void rtc_driver::irq_handler(struct registers* r) {}

uint8_t rtc_driver::get_seconds() { return this->sec; }

uint8_t rtc_driver::get_minutes() { return this->min; }

uint8_t rtc_driver::get_hours() { return this->hours; }

uint8_t rtc_driver::get_day() { return this->day; }

uint8_t rtc_driver::get_month() { return this->month; }

uint8_t rtc_driver::get_year() { return this->year; }

void rtc_driver::update_sec() {
	uint8_t sec = this->read_reg0();
	this->sec = ((sec & 0xF0) >> 1) + ((sec & 0xF0) >> 3) + (sec & 0xf);
}

void rtc_driver::update_min() {
	uint8_t min = this->read_reg2();
	this->min = ((min & 0xF0) >> 1) + ((min & 0xF0) >> 3) + (min & 0xf);
}

void rtc_driver::update_hours() {
	uint8_t hours = this->read_reg4();
	this->hours = ((hours & 0xF0) >> 1) + ((hours & 0xF0) >> 3) + (hours & 0xf);
}

void rtc_driver::update_day() {
	uint8_t day = this->read_reg7();
	this->day = ((day & 0xF0) >> 1) + ((day & 0xF0) >> 3) + (day & 0xf);
}

void rtc_driver::update_month() {
	uint8_t month = this->read_reg8();
	this->month = ((month & 0xF0) >> 1) + ((month & 0xF0) >> 3) + (month & 0xf);
}

void rtc_driver::update_year() {
	uint8_t year = this->read_reg9();
	this->year = ((year & 0xF0) >> 1) + ((year & 0xF0) >> 3) + (year & 0xf);
}

void rtc_driver::update_time() {
	//blocks until update bit is cleared
	//this->wait_to_read_time();
	if (this->is_ready_to_read_time()) {
		uint8_t start = this->sec;
		this->update_sec();
		// If we have no updates to the second yet, then just bail
		if (start == this->sec) return;
		this->update_min();
		this->update_hours();
		this->update_day();
		this->update_month();
		this->update_year();
	}
}
