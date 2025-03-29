#pragma once
#include<kernel/drivers/initrd.hpp>

namespace Initrd {
	extern initrd_driver* driver;
	void init();
	void mount();
}
