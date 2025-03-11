#pragma once
#include<cstdint>
#include<kernel/driver/ps2.hpp>
#include<kernel/driver/driver.hpp>

namespace Drivers {
	void init();
	void load_drivers();
	void dump_ps2_config();
	extern ps2_driver* ps2_driver_ptr;
}
