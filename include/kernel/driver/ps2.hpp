#pragma once
#include<cstdint>
#include<kernel/drivers.h>

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_CMD_PORT 0x64

typedef union ps2_config_u {
	struct {
		uint8_t port1_int : 1;
		uint8_t port2_int : 1;
		uint8_t system_flag : 1;
		uint8_t zero : 1;
		uint8_t port1_clock : 1;
		uint8_t port2_clock : 1;
		uint8_t port1_translate : 1;
		uint8_t zero2 : 1;
	};
	uint8_t ps2_config_byte;
} ps2_config_t;

typedef struct ps2_ident {
	uint8_t byte1;
	uint8_t byte2;
	char* device;
	bool is_kb;
	bool is_mouse;
} ps2_ident_t;

class ps2_driver : public driver_t {
private:
	bool enabled = false;
	bool port1_enabled = false;
	ps2_ident_t port1_device;
	bool port2_enabled = false;
	ps2_ident_t port2_device;

	ps2_ident_t device_types[12] = {
		{ 0x00, 0x00, "PS2 Mouse", false, true },
		{ 0x03, 0x00, "PS2 Mouse + Scroll Wheel", false, true },
		{ 0x04, 0x00, "5 Button PS2 Mouse", false, true },
		{ 0xAB, 0x83, "Generic PS2 Keyboard", true, false },
		{ 0xAB, 0xC1, "Generic PS2 Keyboard", true, false },
		{ 0xAB, 0x84, "Thinkpad/Short keyboard", true, false },
		{ 0xAB, 0x85, "NCD N-97/122-key Host Connected Keyboard", true, false },
		{ 0xAB, 0x86, "122-key Keyboard", true, false },
		{ 0xAB, 0x90, "Japanese 'G' Keyboard", true, false },
		{ 0xAB, 0x91, "Japanese 'P' Keybaord", true, false },
		{ 0xAB, 0x92, "Japanese 'A' Keyboard", true, false },
		{ 0xAC, 0xA1, "NCD Sun Layout Keyboard", true, false }
	};

public:
	ps2_driver();
	ps2_ident_t get_port1_device();
	ps2_ident_t get_port2_device();
	virtual void install(uint64_t idx);
	virtual void irq_handler(struct registers* r);
	bool writeb_device2(uint8_t port, uint8_t value, uint64_t timeout);
	bool input_ready();
	bool output_ready();
	uint8_t poll(uint8_t, uint8_t, uint64_t);
	void enable_port1();
	void enable_port2();
	void reset_port1();
	void reset_port2();
	void dump_config();
	void disable_port1();
	void disable_port2();
	bool controller_ready();
	void test_port1();
	void test_port2();
	void flush_output_buffer();
	void detect_devices();
	void detect_device_port1();
	void detect_device_port2();
	void set_config(ps2_config_t config);
	ps2_config_t get_config();
	void log_config(ps2_config_t config);
	bool self_test();
	bool enable_dual_channel_if_avail();
};
