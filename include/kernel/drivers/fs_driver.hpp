#pragma once
#include<kernel/drivers/driver.hpp>
#include<kernel/vfs/inode.hpp>
//#include<kernel/vfs/vnode.hpp>

class fs_driver_t : public driver_t {
public:
	fs_driver_t() {
		this->desc = FILESYSTEM_DRIVER;
	}
	virtual void install() = 0;
	virtual void write(uint8_t* data, uint64_t ino) = 0;
	virtual size_t read(inode_t* ino, void* buffer, size_t count) = 0;
	virtual void mount() = 0;
};
