#include<cstdint>
#include<kernel/driver/ps2.hpp>
#include<kernel/drivers.h>
#include<kernel.h>
#include<kernel/driver/driver.hpp>
#include<cstdint>

namespace Drivers {
	ps2_driver* ps2_driver_ptr = nullptr;

	void init() {
		//TODO: remove idx from install arguments
		ps2_driver_ptr->install(0);
	}

	void load_drivers() {
		ps2_driver* ps2 = new ps2_driver();
		ps2_driver_ptr = ps2;
	}

	void dump_ps2_config() {
		ps2_driver_ptr->dump_config();
	}
}

