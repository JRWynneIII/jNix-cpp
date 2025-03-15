#pragma once
#include<cstdint>
#include<kernel/devices/devicetree.h>

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
	uint8_t trans_byte1;
	uint8_t trans_byte2;
	char* device;
	bool is_kb;
	bool is_mouse;
	device_desc_t desc;
} ps2_ident_t;

class ps2_driver : public driver_t {
private:
	//TODO: Add support for scancode set 2 and disable translation
	//TODO: Combine keyboard_driver with ps2_driver
	//	Add the keyboard and mouse to a global 'Devices' tree
	//TODO: this is garbage, clean this up!
	uint8_t scancodes_set2[128] = {};
	uint8_t scancodes_set2_upper[128] = {};
	uint8_t scancodes_set1[128] = {
		0,27,'1','2','3','4','5','6','7','8',
		'9','0','-','=','\b',//  Backspace 
		'\t', //  Tab 
		'q','w','e','r',//  19 
		't','y','u','i','o','p','[',']','\n',//  Enter key 
		0,//  29   - Control 
		'a','s','d','f','g','h','j','k','l',';',//  39 
		'\'','`',90,//  Left shift 
		'\\','z','x','c','v','b','n',//  49 
		'm',',','.','/',91,//  Right shift 
		'*',
		0, //  Alt 
		' ',  //  Space bar 
		0,  //  Caps lock 
		0,  //  59 - F1 key ... > 
		0,   0,   0,   0,   0,   0,   0,   0,
		0,  //  < ... F10 
		0,  //  69 - Num lock
		0,  //  Scroll Lock 
		0,  //  Home key 
		0,  //  Up Arrow 
		0,  //  Page Up 
		'-',
		0,  //  Left Arrow 
		0,
		0,  //  Right Arrow 
		'+',
		0,  //  79 - End key
		0,  //  Down Arrow 
		0,  //  Page Down 
		0,  //  Insert Key 
		0,  //  Delete Key 
		0,   0,   0,
		0,  //  F11 Key 
		0,  //  F12 Key 
		0,  //  All other keys are undefined 
	};
	
	uint8_t scancodes_set1_upper[128] = {
		0,27,'!','@','#','$','%','^','&','&', //9
		'(',')','_','+','\b', //Backspace
		'\t', //Tab
		'Q','W','E','R', //19
		'T','Y','U','I','O','P','{','}','\n', //Enterkey
		0, //29-Control
		'A','S','D','F','G','H','J','K','L',':', //39
		'\"','~',90, //Leftshift
		'|','Z','X','C','V','B','N', //49
		'M','<','>','?',91, //Rightshift
		'*',
		0, //Alt
		' ', //Spacebar
		0, //Capslock
		0, //59-F1key...>
		0,0,0,0,0,0,0,0,
		0, //<...F10
		0, //69-Numlock
		0, //ScrollLock
		0, //Homekey
		0, //UpArrow
		0, //PageUp
		'-',
		0, //LeftArrow
		0,
		0, //RightArrow
		'+',
		0, //79-Endkey
		0, //DownArrow
		0, //PageDown
		0, //InsertKey
		0, //DeleteKey
		0,0,0,
		0, //F11Key
		0, //F12Key
		0, //Allotherkeysareundefined
	};
	bool shift_pressed = false;
	bool port1_enabled = false;
	ps2_ident_t port1_device;
	bool port2_enabled = false;
	ps2_ident_t port2_device;
	uint64_t kb_irq = 1;
	uint64_t mouse_irq = 12;

	ps2_ident_t device_types[12] = {
		{ 0x00, 0x00, 0x00, 0x00,"PS2 Mouse", false, true, MOUSE },
		{ 0x03, 0x00, 0x03, 0x00,"PS2 Mouse + Scroll Wheel", false, true, MOUSE },
		{ 0x04, 0x00, 0x04, 0x00,"5 Button PS2 Mouse", false, true, MOUSE },
		{ 0xAB, 0x83, 0xAB, 0x41,"Generic PS2 Keyboard", true, false, KEYBOARD },
		{ 0xAB, 0xC1, 0xAB, 0xC1,"Generic PS2 Keyboard", true, false, KEYBOARD },
		{ 0xAB, 0x84, 0xAB, 0x54,"Thinkpad/Short keyboard", true, false, KEYBOARD },
		{ 0xAB, 0x85, 0xAB, 0x85,"NCD N-97/122-key Host Connected Keyboard", true, false, KEYBOARD },
		{ 0xAB, 0x86, 0xAB, 0x86,"122-key Keyboard", true, false, KEYBOARD },
		{ 0xAB, 0x90, 0xAB, 0x90,"Japanese 'G' Keyboard", true, false, KEYBOARD },
		{ 0xAB, 0x91, 0xAB, 0x91,"Japanese 'P' Keybaord", true, false, KEYBOARD },
		{ 0xAB, 0x92, 0xAB, 0x92,"Japanese 'A' Keyboard", true, false, KEYBOARD },
		{ 0xAC, 0xA1, 0xAC, 0xA1,"NCD Sun Layout Keyboard", true, false, KEYBOARD }
	};

public:
	ps2_driver();
	ps2_ident_t get_port1_device();
	ps2_ident_t get_port2_device();
	virtual void install();
	virtual void irq_handler(struct registers* r);
	void keyboard_irq_handler(struct registers* r);
	void mouse_irq_handler(struct registers* r);
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
