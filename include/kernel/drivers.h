#pragma once
#include<cstdint>
#include<kernel/driver/driver.hpp>
#include<kernel/driver/keyboard.hpp>

namespace Drivers {
	void init();
	void load_drivers();
	void dump_ps2_config();
	extern keyboard_driver* keyboard_driver_ptr;
	extern driver_t* driver_list;
	extern uint64_t kb_driver_idx;
	extern uint64_t ps2_driver_idx;
}
