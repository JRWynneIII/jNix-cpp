#pragma once
#include<cstdint>
#include<kernel/driver/ps2.hpp>
#include<kernel/driver/driver.hpp>

namespace Drivers {
	void init();
	void load_drivers();
	void dump_ps2_config();
	pit_driver* get_timer_driver();
	ps2_driver* get_ps2_driver();
}
