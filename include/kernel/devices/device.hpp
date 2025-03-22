#pragma once
#include<kernel/drivers/driver.hpp>
#include<vector.hpp>

typedef enum device_desc {
	KEYBOARD,
	MOUSE,
	BLOCKDEV,
	CHARDEV,
	TIMER,
	CLOCK,
	CONSOLE,
	UNKNOWN,
} device_desc_t;

class Device {
private:
	driver_t* driver;
	device_desc_t desc;
	char* path;
public:
	Device();
	Device(driver_t* d, device_desc_t des, char* p) : driver(d), desc(des), path(p) {}
	void set_driver(driver_t* d);
	driver_t* get_driver();
	void set_device_type(device_desc_t d);
	device_desc_t get_device_type();
	void set_path(char* n);
	char* get_path();
};
