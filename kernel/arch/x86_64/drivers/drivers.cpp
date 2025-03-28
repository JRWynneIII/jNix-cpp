#include<cstdint>
#include<kernel/interrupts.h>
#include<kernel/drivers/ps2.hpp>
#include<kernel/drivers/pit.hpp>
#include<kernel/drivers/rtc.hpp>
#include<kernel/drivers/framebuffer.hpp>
#include<kernel/drivers/driver_api.hpp>
#include<kernel.h>
#include<kernel/drivers/driver.hpp>
#include<kernel/drivers/initrd.hpp>
#include<cstdint>
#include<vector.hpp>

namespace Drivers {
	vector<driver_t*>& drivers() {
		static vector<driver_t*>* d = new vector<driver_t*>();
		return *d;
	}
	ps2_driver* ps2_driver_ptr = nullptr;
	pit_driver* pit_driver_ptr = nullptr;
	rtc_driver* rtc_driver_ptr = nullptr;
	framebuffer_driver* fb_driver_ptr = nullptr;

	framebuffer_driver* get_framebuffer_driver() {
		for( auto d : drivers() ) {
			if (d->desc == FRAMEBUFFER_DRIVER) return static_cast<framebuffer_driver*>(d);
		}
		return nullptr;
	}

	rtc_driver* get_rtc_driver() {
		for( auto d : drivers() ) {
			if (d->desc == RTC_DRIVER) return static_cast<rtc_driver*>(d);
		}
		return nullptr;
	}

	pit_driver* get_timer_driver() {
		for (int i = 0; i < drivers().length() ; i++) {
			driver_t* d = drivers().at(i);
			if (d->desc == TIMER_DRIVER) return static_cast<pit_driver*>(d);
		}
		return nullptr;
	}

	initrd_driver* get_initrd_driver() {
		for (int i = 0; i < drivers().length() ; i++) {
			driver_t* d = drivers().at(i);
			if (d->desc == FILESYSTEM_DRIVER) return static_cast<initrd_driver*>(d);
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

		auto rtc = get_rtc_driver();
		if (rtc != nullptr) rtc->install();
		else logfk(ERROR, "Could not find rtc driver during initialize!\n");

		auto initrd = get_initrd_driver();
		if (initrd != nullptr) initrd->install();
		else logfk(ERROR, "Could not find initrd driver during initialize!\n");
	}

	void load_drivers() {
		ps2_driver* ps2 = new ps2_driver();
		drivers().push_back(ps2);

		pit_driver* pit = new pit_driver();
		drivers().push_back(pit);

		rtc_driver* rtc = new rtc_driver();
		drivers().push_back(rtc);

		initrd_driver* tfs = new initrd_driver();
		drivers().push_back(tfs);

		//Finish initialization of Console device and framebuffer_driver
		framebuffer_driver* fb = &FrameBuffer::driver();
		Devices::add_device(fb, CONSOLE, "console");
	}

	void dump_ps2_config() {
		ps2_driver_ptr->dump_config();
	}
}

