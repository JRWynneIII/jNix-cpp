#pragma once
#include<cstdint>
#include<kernel/drivers/ps2.hpp>
#include<kernel/drivers/pit.hpp>
#include<kernel/drivers/rtc.hpp>
#include<kernel/drivers/driver.hpp>

namespace Drivers {
	void init();
	void load_drivers();
	void dump_ps2_config();
	ps2_driver* get_ps2_driver();
	pit_driver* get_timer_driver();
	rtc_driver* get_rtc_driver();
}
