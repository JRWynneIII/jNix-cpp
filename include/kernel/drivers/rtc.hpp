#pragma once

class rtc_driver : public driver_t {
private:
	device_desc_t device_desc;
	uint8_t hours;
	uint8_t min;
	uint8_t sec;
	uint8_t day;
	uint8_t month;
	uint8_t year;
public:
	rtc_driver();
	virtual void install();
	virtual void irq_handler(struct registers* r);
	bool is_ready_to_read_time();
	uint8_t get_seconds();
	uint8_t get_minutes();
	uint8_t get_hours();
	uint8_t get_day();
	uint8_t get_month();
	uint8_t get_year();
	void update_sec();
	void update_min();
	void update_hours();
	void update_day();
	void update_month();
	void update_year();
	void update_time();
	void wait_to_read_time();
	void write_regA(uint8_t byte);
	void write_regB(uint8_t byte);
	void write_regC(uint8_t byte);
	uint8_t read_reg0();
	uint8_t read_reg2();
	uint8_t read_reg4();
	uint8_t read_reg7();
	uint8_t read_reg8();
	uint8_t read_reg9();
	uint8_t read_regA();
	uint8_t read_regB();
	uint8_t read_regC();
	void update();
	void enable_interrupt();
};
