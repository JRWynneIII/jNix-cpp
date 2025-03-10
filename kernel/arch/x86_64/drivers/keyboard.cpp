#include<cstdint>
#include<kernel/ptr.hpp>
#include<kernel/driver/ps2.hpp>
#include<kernel/driver/driver.hpp>
#include<kernel/driver/keyboard.hpp>
#include<kernel.h>
#include<port.h>
#include<interrupts.h>
#include<kernel/streams.h>
#include<kernel/drivers.h>
#include<kernel/driver/ps2.hpp>
#include<cstdint>
#include<string.h>

//TODO: Clean this up by creating a 'device tree' and assigning this driver to the device itself. 
//	depending on what root device driver owns this (for insatnce the ps2 or usb driver), assign
//	the irq handler and irq no appropriately
void keyboard_driver::irq_handler(struct registers* r) {
	switch(this->irq_handler_override) {
		case PS2:
			this->ps2_irq_handler(r);
		default:
			this->ps2_irq_handler(r);
	}
}

void keyboard_driver::ps2_irq_handler(struct registers* r) {
	uint8_t scancode = inportb(PS2_DATA_PORT);
	//Toggles shift key
	if (scancode == 0xAA || scancode == 0xB6) {
		this->shift_pressed = false;
		return;
	} else if (scancode == 0x36 || scancode == 0x2A) {
		this->shift_pressed = true;
		return;
	}
	
	// Toggle the right character set based upon shift
	uint8_t* chars = this->scancodes_set1;
	if (this->shift_pressed) chars = this->scancodes_set1_upper;

	// If we don't understand the scancode, just skip it
	if (chars[scancode] == 0) return;

	//Check top bit to see if key has been released
	if ((scancode & 0x80) != 0) return;

	Streams::stdin.append(chars[scancode]);
}

void kb_driver_wrapper(struct registers* r) {
	Drivers::keyboard_driver_ptr->irq_handler(r);
}

void keyboard_driver::install(uint64_t idx) {
	logfk(KERNEL, "Installing keyboard driver\n");
	this->set_name("kb");

	driver_t* ps2 = Drivers::driver_list;
	while(ps2 != nullptr) {
		logfk(KERNEL, "Found driver %s\n", ps2->get_name());
		//TODO: make a strcmp routine, or convert these to a custom "String" type
		if (strcmp(ps2->get_name(), "ps2")) {
			break;
		}
		ps2 = ps2->get_next();
	}

	if (ps2 == nullptr) {
		logfk(ERROR, "Could not find ps2 controller driver!\n");
	} else {
		ps2_driver* d = static_cast<ps2_driver*>(ps2);
		if (d->get_port1_device().is_kb || d->get_port2_device().is_kb) {
			logfk(KERNEL, "Setting PS2 handler for irq_handler for keyboard_driver\n");
			this->irq_handler_override = PS2;
		}
		Interrupts::install_handler(this->irq_no, kb_driver_wrapper);
	}
}

keyboard_driver::keyboard_driver() {
	this->irq_no = 1;
	this->irq_handler_override = DEFAULT;
}
