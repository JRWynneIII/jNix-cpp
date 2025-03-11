#pragma once
#include<cstdint>
//#include<kernel/ptr.hpp>

class driver_t {
private:
	driver_t* next;
	char* name;
	uint64_t attached_devices;
public:
	uint64_t irq_no;

	driver_t() {
		this->next = nullptr;
		this->irq_no = 999;
		this->attached_devices = 0;
	}

	virtual void install(uint64_t idx) = 0;
	virtual void irq_handler(struct registers* r) = 0;

	driver_t* get_next() {
		return this->next;
	}

	void add_device() {
		this->attached_devices++;
	}

	void rm_device() {
		this->attached_devices--;
	}

	uint64_t num_devices() {
		return this->attached_devices;
	}

	void set_next(driver_t* d) {
		this->next = d;
	}

	char* get_name() {
		return this->name;
	}

	void set_name(char* name) {
		this->name = name;
	}
};
