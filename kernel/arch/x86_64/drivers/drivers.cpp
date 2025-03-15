#include<cstdint>
#include<kernel/interrupts.h>
#include<kernel/driver/ps2.hpp>
#include<kernel/driver/pit.hpp>
#include<kernel/drivers.h>
#include<kernel.h>
#include<kernel/driver/driver.hpp>
#include<cstdint>
#include<vector.hpp>

namespace Drivers {
	vector<driver_t*>& drivers() {
		static vector<driver_t*>* d = new vector<driver_t*>();
		return *d;
	}
	ps2_driver* ps2_driver_ptr = nullptr;
	pit_driver* pit_driver_ptr = nullptr;

	pit_driver* get_timer_driver() {
		for (int i = 0; i < drivers().length() ; i++) {
			driver_t* d = drivers().at(i);
			if (d->desc == TIMER_DRIVER) return static_cast<pit_driver*>(d);
		}
		return nullptr;
	}

	ps2_driver* get_ps2_driver() {
		for (int i = 0; i < drivers().length() ; i++) {
			driver_t* d = drivers().at(i);
			if (d->desc == PS2_DRIVER) return static_cast<ps2_driver*>(d);
		}
		return nullptr;
	}

	void init() {
		auto ps2 = get_ps2_driver();
		if (ps2 != nullptr) ps2->install();
		else logfk(ERROR, "Could not find PS2 driver during initialize!\n");
		auto timer = get_timer_driver();
		if (timer != nullptr) timer->install();
		else logfk(ERROR, "Could not find timer driver during initialize!\n");
	}

	void load_drivers() {
		ps2_driver* ps2 = new ps2_driver();
		drivers().push_back(ps2);

		pit_driver* pit = new pit_driver();
		drivers().push_back(pit);
	}

	void dump_ps2_config() {
		ps2_driver_ptr->dump_config();
	}
}

