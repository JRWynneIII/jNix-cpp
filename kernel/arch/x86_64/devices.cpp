#include<cstdint>
#include<interrupts.h>
#include<kernel/devices.hpp>
#include<vector.hpp>
#include<kernel.h>
#include<kernel/drivers.h>
#include<string.h>

namespace Devices {
	vector<Device*>& device_tree() {
		static vector<Device*>* tree = new vector<Device*>();
		return *tree;
	}

	uint64_t get_new_subsystem_idx(driver_t* driver) {
		driver->add_device();
		return driver->num_devices();
	}

	char* format_path(char* p) {
		for (int i=0; i < strlen(p); i++) {
			if (p[i] == ' ') {
				p[i] = '_';
			} else {
				if (p[i] > 0x40 && p[i] < 0x5B) {
					p[i] |= 0x60;
				} else {
					continue;
				}
			}
		}
		return p;
	}

	void add_device(driver_t* driver, device_desc_t desc, char* name) {
		//TODO: Change name so that it is lowercase and ' ' is _
		char* path = sprintf("%s.%s.%d", driver->get_name(), name, get_new_subsystem_idx(driver));
		path = format_path(path);
		Device* dev = new Device(driver, desc, path);
		device_tree().push_back(dev);
	}

	void dump_device_tree() {
		logfk(KERNEL, "Device Tree:\n");
		for (int i = 0; i < device_tree().length() ; i++) {
			Device* d = device_tree().at(i);
			printfk("\tDevice: %s, %d, %s\n", d->get_driver()->get_name(), d->get_device_type(), d->get_path());
		}
	}

	void rm_device(char* path) {
		for (int i = 0; i < device_tree().length(); i++) {
			Device* d = device_tree().at(i);
			if (d->get_path() == path) {
				device_tree().del(i);
				break;
			}
		}
	}

	vector<Device*>* get_devices_by_driver(char* driver_name) {
		vector<Device*>* devs = new vector<Device*>();
		for (int i = 0; i < device_tree().length(); i++) {
			driver_t* d = device_tree().at(i)->get_driver();
			if (strcmp(d->get_name(), driver_name)) devs->push_back(device_tree().at(i));
		}
		return devs;
	}

	//Each device will have a 'path' like ps2.keyboard.1, usb.flash.0, etc
	Device* get_device_by_path(char* path) {
		for (int i = 0; i < device_tree().length(); i++) {
			Device* d = device_tree().at(i);
			if (strcmp(d->get_path(), path)) {
				return d;
			}
		}
		return nullptr;
	}
}


Device::Device() {
	this->desc = UNKNOWN;
	this->driver = nullptr;
	this->path = "";
}

void Device::set_driver(driver_t* d) {
	this->driver = d;
}

driver_t* Device::get_driver() {
	return this->driver;
}

void Device::set_device_type(device_desc_t d) {
	this->desc = d;
}

device_desc_t Device::get_device_type() {
	return this->desc;
}

void Device::set_path(char* n) {
	this->path = n;
}

char* Device::get_path() {
	return this->path;
}

