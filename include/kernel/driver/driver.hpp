#pragma once
//#include<kernel/ptr.hpp>

class driver_t {
private:
	driver_t* next;
public:
	uint64_t irq_no;

	driver_t() {
		this->next = nullptr;
		this->irq_no = 999;
	}

	virtual void install(uint64_t idx) = 0;
	virtual void irq_handler(struct registers* r) = 0;

	driver_t* get_next() {
		return this->next;
	}

	void set_next(driver_t* d) {
		this->next = d;
	}
};
