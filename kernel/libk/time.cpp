#include<cstdint>
#include<kernel.h>
#include<kernel/interrupts.h>
#include<kernel/drivers/driver_api.hpp>
#include<kernel/devices/device_api.hpp>

namespace Kernel {
	namespace Time {
		void sleep(uint64_t milli) {
			Device* timer = Devices::get_device_by_path("pit.programmable_interrupt_timer.1");
			pit_driver* driver = static_cast<pit_driver*>(timer->get_driver());
			uint64_t start = driver->get_ticks();
			uint64_t end = start + milli;
			while(driver->get_ticks() < end) {}
			return;
		}
	}
}
