#include<cstdint>
#include<kernel/ptr.hpp>
#include<kernel/driver/driver.hpp>
#include<kernel/driver/keyboard.hpp>
#include<kernel.h>
#include<port.h>
#include<interrupts.h>

#define KB_PORT 0x60

namespace Streams {
	//TODO: I WOULD KILL FOR A VECTOR
	//ptr_t<char> stdin = new ptr_t<char>(128*sizeof(char));
	ptr_t<uint8_t> stdin = ptr_t<uint8_t>(128*sizeof(uint8_t));
}


void keyboard_driver::irq_handler(struct registers* r) {
	uint8_t scancode = inportb(KB_PORT);
	//Toggles shift key
	if (scancode == 0xAA || scancode == 0xB6) {
		this->shift_pressed = false;
		return;
	} else if (scancode == 0x36 || scancode == 0x2A) {
		this->shift_pressed = true;
		return;
	}
	
	// Toggle the right character set based upon shift
	uint8_t* chars = this->scancodes;
	if (this->shift_pressed) chars = this->scancodes_upper;

	// If we don't understand the scancode, just skip it
	if (chars[scancode] == 0) return;

	//Check top bit to see if key has been released
	if ((scancode & 0x80) != 0) return;

	Streams::stdin.append(chars[scancode]);
}

uint8_t getch() {
	//pop first char from the stream
	uint8_t c = Streams::stdin.at(0);
	//Poll until we have a character in the stream
	while(c == 0) {
		c = Streams::stdin.at(0);
	}
	// Bump the queue down 1
	Streams::stdin.pop();
}


namespace Drivers {
	// TODO: when you port a stdlib, change this from a linked list to a vector. PLEASE
	driver_t* driver_list;
	uint64_t kb_driver_idx = 0;

	void register_driver(driver_t* d) {
		if (driver_list == nullptr) {
			driver_list = d;
			return;
		}

		driver_t* cur = driver_list;
		while(cur->get_next() != nullptr) {
			cur = cur->get_next();
		}
		cur->set_next(d);
	}

	void init() {
		driver_t* cur = driver_list;
		// If we only have 1 in the list, install it
		uint64_t idx = 0;
		if (cur != nullptr && cur->get_next() == nullptr) {
			cur->install(idx);
			cur = cur->get_next();
		}

		while(cur != nullptr && cur->get_next() != nullptr) {
			cur->install(idx);
			cur = cur->get_next();
		}
	}

	void load_drivers() {
		keyboard_driver* kb = new keyboard_driver();
		register_driver(kb);
		//TODO: more drivers go here
	}
}

void kb_driver_wrapper(struct registers* r) {
	Drivers::driver_list[Drivers::kb_driver_idx].irq_handler(r);
}

void keyboard_driver::install(uint64_t idx) {
	logfk(KERNEL, "Installing keyboard driver\n");
	Drivers::kb_driver_idx = idx;
	Interrupts::install_handler(this->irq_no, kb_driver_wrapper);
}

keyboard_driver::keyboard_driver() {
	this->irq_no = 1;
}
