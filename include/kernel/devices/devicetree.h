#pragma once
#include<vector.hpp>
#include<kernel/driver/driver.hpp>
#include<kernel/devices/devices.hpp>

namespace Devices {
	vector<Device*>& device_tree();
	void add_device(driver_t* driver, device_desc_t desc, char* name);
	void dump_device_tree();
	uint64_t get_new_subsystem_idx(driver_t* driver);
	void rm_device(char* path);
	vector<Device*>* get_devices_by_driver(char* driver_name);
	Device* get_device_by_path(char* path);
}
