#include<kernel/drivers/driver.hpp>
#include<kernel/drivers/initrd.hpp>

namespace Initrd {
	initrd_driver* driver;

	void init() {
 		driver = new initrd_driver();
	}

	void mount() {
		driver->install();
		if (driver->enabled) {
			driver->mount();
		} else {
			logfk(ERROR, "Could not mount initrd! Driver did not successfully initialize\n");
			return;
		}
	}
}
