#pragma once
#include<kernel/drivers/driver.hpp>
#include<kernel/vfs/vnode.hpp>

class fs_driver_t : public driver_t {
public:
	fs_driver_t() {
		this->desc = FILESYSTEM_DRIVER;
	}
	virtual void install() = 0;
	virtual void write(uint8_t* data, uint64_t ino) = 0;
	virtual uint8_t* read(uint64_t ino) = 0;
	virtual void mount() = 0;
};
