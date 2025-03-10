#include<cstdint>
#include<kernel.h>
#include<kernel/driver/driver.hpp>
#include<kernel/driver/keyboard.hpp>
#include<kernel/driver/ps2.hpp>
#include<cstdint>

namespace Drivers {
	// TODO: when you port a stdlib, change this from a linked list to a vector. PLEASE
	driver_t* driver_list = nullptr;
	ps2_driver* ps2_driver_ptr = nullptr;
	keyboard_driver* keyboard_driver_ptr = nullptr;
	uint64_t ps2_driver_idx = 0;

	void register_driver(driver_t* d) {
		driver_t* cur = driver_list;
		if (cur == nullptr) {
			driver_list = d;
			return;
		}

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
			idx++;
		}

		while(cur != nullptr) {
			cur->install(idx);
			cur = cur->get_next();
			idx++;
		}
	}

	void load_drivers() {
		ps2_driver* ps2 = new ps2_driver();
		ps2_driver_ptr = ps2;
		register_driver(ps2);
		keyboard_driver* kb = new keyboard_driver();
		keyboard_driver_ptr = kb;
		register_driver(kb);
		//TODO: more drivers go here
	}

	void dump_ps2_config() {
		ps2_driver_ptr->dump_config();
	}
}

