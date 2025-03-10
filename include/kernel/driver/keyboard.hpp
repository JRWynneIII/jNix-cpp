#pragma once

class keyboard_driver : public driver_t {
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
	enum irq_handler_override_e {
		PS2,
		USB,
		DEFAULT
	} irq_handler_override;

public:
	keyboard_driver();
	virtual void install(uint64_t idx);
	virtual void irq_handler(struct registers* r);
	void ps2_irq_handler(struct registers* r);
};
