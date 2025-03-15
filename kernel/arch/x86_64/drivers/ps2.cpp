#include<cstdint>
#include<kernel/interrupts.h>
#include<kernel/drivers.h>
#include<kernel/driver/driver.hpp>
#include<kernel/streams.h>
#include<kernel/port.h>
#include<kernel/driver/ps2.hpp>
#include<kernel.h>
#include<kernel/devicetree.h>

ps2_driver::ps2_driver() {
	this->desc = PS2_DRIVER;
}

void ps2_driver::set_config(ps2_config_t config) {
	//Wait for controller to be ready
	while(!this->input_ready()) {}
	//logfk(KERNEL, "PS2: setting config: \n");
	//ps2_driver::log_config(config);
	outportb(PS2_CMD_PORT, 0x60);
	outportb(PS2_DATA_PORT, config.ps2_config_byte);
}

void ps2_driver::log_config(ps2_config_t config) {
	printfk("\tport1_int: %d", config.port1_int);
	printfk("\tport2_int: %d", config.port2_int);
	printfk("\tsystem_flag: %d", config.system_flag);
	//printfk("\tzero: %d", config.zero);
	printfk("\tport1_clock: %d", config.port1_clock);
	printfk("\tport2_clock: %d", config.port2_clock);
	printfk("\tport1_translate: %d\n", config.port1_translate);
	//printfk("\tzero2: %d\n", config.zero2);
}

ps2_ident_t ps2_driver::get_port1_device() {
	return this->port1_device;
}

ps2_ident_t ps2_driver::get_port2_device() {
	return this->port2_device;
}

ps2_config_t ps2_driver::get_config() {
	while(!this->input_ready()) {}
	outportb(PS2_CMD_PORT, 0x20);
	while(!this->output_ready()) {}
	uint8_t conf = inportb(PS2_DATA_PORT);
	//logfk(KERNEL, "PS2: got config: %d\n", conf);
	ps2_config_t config;
	config.ps2_config_byte = conf;
	//ps2_driver::log_config(config);
	return config;
}

void ps2_driver::detect_devices() {
	if (this->port1_enabled)
		this->detect_device_port1();
	if (this->port2_enabled)
		this->detect_device_port2();
}

bool ps2_driver::writeb_device2(uint8_t port, uint8_t value, uint64_t timeout) {
	outportb(PS2_CMD_PORT, 0xD4);
	uint64_t cur = 0;
	while(!this->input_ready()) {
		if (cur >= timeout) break;
		cur++;
	}
	if (cur >= timeout) return false;
	outportb(port, value);
	return true;
}

void ps2_driver::detect_device_port2() {
	//Port1 detect
	uint64_t poll_timeout = 1000;
	int timeout = 0;
	//Wait for input_buffer to be clear (may need to replace this with something to wait for bit 1 to be clear)
	while(!this->input_ready()) {
		if (timeout >= poll_timeout) break;
		timeout++;
	}
	if (!this->input_ready()) {
		logfk(ERROR, "PS2 controller input buffer not ready; timeout reached\n");
		this->port2_enabled = false;
		return;
	}

	//Disable scanning
	bool success = this->writeb_device2(PS2_DATA_PORT, 0xF5, poll_timeout);
	if (!success) {
		logfk(ERROR, "Could not communicate with PS2 device 2!; timeout reached (disable scanning)\n");
		this->port2_enabled = false;
		return;
	}
	uint8_t ack = this->poll(PS2_DATA_PORT, 0xFA, poll_timeout);
	if (ack != 0xFA) {
		logfk(ERROR, "Did not rx ACK (0xFA) from PS2 device 2 while disabling scanning; timeout reached, rx'd: %d\n", ack);
		this->port2_enabled = false;
		return;
	}

	//Request identify for port2
	success = this->writeb_device2(PS2_DATA_PORT, 0xF2, poll_timeout);
	if (!success) {
		logfk(ERROR, "Could not communicate with PS2 device 2!; timeout reached (request identify)\n");
		this->port2_enabled = false;
		return;
	}
	ack = this->poll(PS2_DATA_PORT, 0xFA, poll_timeout);
	if (ack != 0xFA) {
		logfk(ERROR, "Did not rx ACK (0xFA) from PS2 device 2 during identify; timeout reached, rx'd: %d\n", ack);
		this->port2_enabled = false;
		return;
	}

	uint8_t ident_port2_byte1 = inportb(PS2_DATA_PORT);
	//Wait for next byte
	timeout = 0;
	while(ident_port2_byte1 == 0xFA) {
		if (timeout > poll_timeout) break;
		ident_port2_byte1 = inportb(PS2_DATA_PORT);
		timeout++;
	}

	uint8_t ident_port2_byte2 = inportb(PS2_DATA_PORT);
	timeout = 0;
	while(ident_port2_byte2 != ident_port2_byte1) {
		if (timeout > poll_timeout) break;
		ident_port2_byte2 = inportb(PS2_DATA_PORT);
		timeout++;
	}

	//Re-enable scanning
	success = this->writeb_device2(PS2_DATA_PORT, 0xF4, poll_timeout);
	if (!success) {
		logfk(ERROR, "Could not communicate with PS2 device 2!; timeout reached (enable scanning)\n");
		this->port2_enabled = false;
		return;
	}

	for (int i = 0; i < 12; i++) {
		ps2_ident_t cur = this->device_types[i];
		if (ident_port2_byte1 == cur.byte1 && ident_port2_byte2 == cur.byte2) {
			this->port2_device = cur;
			break;
		} else if (ident_port2_byte1 == cur.trans_byte1 && ident_port2_byte2 == cur.trans_byte2) {
			this->port2_device = cur;
			break;
		}
	}

	logfk(KERNEL, "PS2 port 2 device: %s\n", this->port2_device.device);
	Devices::add_device(this, this->port2_device.desc, this->port2_device.device);
}

void ps2_driver::reset_port1() {
	uint64_t poll_timeout = 1000;
	int timeout = 0;
	//Wait for input_buffer to be clear (may need to replace this with something to wait for bit 1 to be clear)
	while(!this->input_ready()) {
		if (timeout >= poll_timeout) break;
		timeout++;
	}
	if (!this->input_ready()) {
		logfk(ERROR, "PS2 controller input buffer not ready; timeout reached\n");
		return;
	}
	outportb(PS2_DATA_PORT, 0xFF);
	uint8_t ack = this->poll(PS2_DATA_PORT, 0xFA, poll_timeout);
	if (ack != 0xFA) {
		logfk(ERROR, "Did not rx ACK (0xFA) from PS2 device 1 while resetting; timeout reached, rx'd: %x\n", ack);
		this->port1_enabled = false;
		return;
	}
	uint8_t st = this->poll(PS2_DATA_PORT, 0xAA, poll_timeout);
	if (st != 0xAA) {
		logfk(ERROR, "Did not rx Self-Test Pass (0xAA) from PS2 device 1 while resetting or timeout reached, rx'd: %x\n", st);
		this->port1_enabled = false;
		return;
	}
	logfk(KERNEL, "PS2 port 1 reset and passed self test\n");
}

void ps2_driver::reset_port2() {
	uint64_t poll_timeout = 1000;
	int timeout = 0;
	//Wait for input_buffer to be clear (may need to replace this with something to wait for bit 1 to be clear)
	while(!this->input_ready()) {
		if (timeout >= poll_timeout) break;
		timeout++;
	}
	if (!this->input_ready()) {
		logfk(ERROR, "PS2 controller input buffer not ready; timeout reached\n");
		this->port2_enabled = false;
		return;
	}
	this->writeb_device2(PS2_DATA_PORT, 0xFF, poll_timeout);
	uint8_t ack = this->poll(PS2_DATA_PORT, 0xFA, poll_timeout);
	if (ack != 0xFA) {
		logfk(ERROR, "Did not rx ACK (0xFA) from PS2 device 1 while resetting; timeout reached, rx'd: %d\n", ack);
		this->port2_enabled = false;
		return;
	}
	uint8_t st = this->poll(PS2_DATA_PORT, 0xAA, poll_timeout);
	if (st != 0xAA) {
		logfk(ERROR, "Did not rx Self-Test Pass (0xAA) from PS2 device 2 while resetting or timeout reached, rx'd: %x\n", st);
		this->port2_enabled = false;
		return;
	}
	logfk(KERNEL, "PS2 port 2 reset and passed self test\n");
}

void ps2_driver::detect_device_port1() {
	//Port1 detect
	uint64_t poll_timeout = 1000;
	int timeout = 0;
	//Wait for input_buffer to be clear (may need to replace this with something to wait for bit 1 to be clear)
	while(!this->input_ready()) {
		if (timeout >= poll_timeout) break;
		timeout++;
	}
	if (!this->input_ready()) {
		logfk(ERROR, "PS2 controller input buffer not ready; timeout reached\n");
		this->port1_enabled = false;
		return;
	}

	//Disable scanning
	outportb(PS2_DATA_PORT, 0xF5);
	uint8_t ack = this->poll(PS2_DATA_PORT, 0xFA, poll_timeout);
	if (ack != 0xFA) {
		logfk(ERROR, "Did not rx ACK (0xFA) from PS2 device 1 while disabling scanning; timeout reached, rx'd: %d\n", ack);
		this->port1_enabled = false;
		return;
	}

	//Request identify for port1
	outportb(PS2_DATA_PORT, 0xF2);
	ack = this->poll(PS2_DATA_PORT, 0xFA, poll_timeout);
	if (ack != 0xFA) {
		logfk(ERROR, "Did not rx ACK (0xFA) from PS2 device 1 during identify; timeout reached, rx'd: %d\n", ack);
		this->port1_enabled = false;
		return;
	}

	uint8_t ident_port1_byte1 = inportb(PS2_DATA_PORT);
	//Wait for next byte
	timeout = 0;
	while(ident_port1_byte1 == 0xFA) {
		if (timeout > poll_timeout) break;
		ident_port1_byte1 = inportb(PS2_DATA_PORT);
		timeout++;
	}

	uint8_t ident_port1_byte2 = inportb(PS2_DATA_PORT);
	timeout = 0;
	while(ident_port1_byte2 != ident_port1_byte1) {
		if (timeout > poll_timeout) break;
		ident_port1_byte2 = inportb(PS2_DATA_PORT);
		timeout++;
	}

	//Re-enable scanning
	outportb(PS2_DATA_PORT, 0xF4);

	for (int i = 0; i < 12; i++) {
		ps2_ident_t cur = this->device_types[i];
		if (ident_port1_byte1 == cur.byte1 && ident_port1_byte2 == cur.byte2) {
			this->port1_device = cur;
			break;
		} else if (ident_port1_byte1 == cur.trans_byte1 && ident_port1_byte2 == cur.trans_byte2) {
			this->port1_device = cur;
			break;
		}
	}

	logfk(KERNEL, "PS2 port 1 device: %s\n", this->port1_device.device);
	Devices::add_device(this, this->port1_device.desc, this->port1_device.device);
}

uint8_t ps2_driver::poll(uint8_t port, uint8_t value, uint64_t timeout) {
	uint8_t cur = inportb(port);
	uint64_t curtime = 0;

	while(cur != value) {
		if (curtime >= timeout) break;
		cur = inportb(port);
		curtime++;
	}
	return cur;
}

void ps2_kb_driver_wrapper(struct registers* r) {
	ps2_driver* d = Drivers::get_ps2_driver();
	if (d != nullptr) {
		d->keyboard_irq_handler(r);
	} else {
		logfk(ERROR, "Can not find ps2 driver!\n");
	}
}

void ps2_driver::install() {
	logfk(KERNEL, "Installing PS2 controller driver\n");
	//Drivers::ps2_driver_idx = idx;
	this->set_name("ps2");

	this->disable_port1();
	this->disable_port2();

	this->flush_output_buffer();

	ps2_config_t conf = this->get_config();
	conf.port1_int = 0;
	conf.port1_clock = 0;
	conf.port1_translate = 0;
	this->set_config(conf);

	this->self_test();

	bool port2exists = this->enable_dual_channel_if_avail();

	this->test_port1();
	if (port2exists) this->test_port2();

	//If at least 1 port is enabled, enable the driver
	if (this->port1_enabled || this->port2_enabled) this->enabled = true;

	// Enable the ports that passed their self tests
	if (this->port1_enabled) {
		this->enable_port1();
		logfk(KERNEL, "PS2 port 1 enabled\n");
	}
	if (this->port2_enabled) {
		this->enable_port2();
		logfk(KERNEL, "PS2 port 2 enabled\n");
	}
	
	// Enable interuptfs for the ports that passed their self tests
	conf = this->get_config();
	if (this->port1_enabled) {
		conf.port1_int = 1;
		conf.port1_translate = 1;
	}
		
	if (this->port2_enabled)
		conf.port2_int = 1;
	this->set_config(conf);

	//Reset and detect devices 
	//TODO: Reset AND Detect_devices seems to reset the config register?
	this->reset_port1();
	this->reset_port2();

	this->detect_devices();
	this->flush_output_buffer();

	if (this->port1_enabled) {
		Interrupts::install_handler(this->kb_irq, ps2_kb_driver_wrapper);
		// Unmask the irq on the PIC
		this->enable(1);
	}

}

void ps2_driver::dump_config() {
	ps2_config_t config = this->get_config();
	printfk("\tport1_int: %d", config.port1_int);
	printfk("\tport2_int: %d", config.port2_int);
	printfk("\tsystem_flag: %d", config.system_flag);
	//printfk("\tzero: %d", config.zero);
	printfk("\tport1_clock: %d", config.port1_clock);
	printfk("\tport2_clock: %d", config.port2_clock);
	printfk("\tport1_translate: %d\n", config.port1_translate);
	//log_config(conf);
}

bool ps2_driver::enable_dual_channel_if_avail() {
	logfk(KERNEL, "Checking for dual channel PS2 controller\n");
	while(!this->input_ready()) {}
	outportb(PS2_CMD_PORT, 0xA8);
	ps2_config_t config = this->get_config();
	if (config.port2_clock == 0) {
		//Is dual channel
		logfk(KERNEL, "PS2 controller is dual channel\n");
		this->disable_port2();
		//disable 2nd port again
		config.port2_int = 0;
		config.port2_clock = 0;
		this->set_config(config);
		return true;
	} else {
		logfk(KERNEL, "PS2 controller is single channel\n");
		return false;
	}
}

bool ps2_driver::controller_ready() {
	//TODO: maybe replace this with input_ready()
	//logfk(KERNEL, "Checking if PS2 controller is ready for commands\n");
	uint8_t status = inportb(PS2_STATUS_PORT);
	//logfk(KERNEL, "Responded: %d\n", status);
	if (status & 0b00000010 == 0) return false;
	return true;
}

bool ps2_driver::self_test() {
	// TODO might need to read the config first and restore it?
	logfk(KERNEL, "Running PS2 controller self test...\n");
	while(!this->input_ready()) {}
	outportb(PS2_CMD_PORT, 0xAA);
	while(!this->output_ready()) {}
	uint8_t status = inportb(PS2_DATA_PORT);
	if (status != 0x55) {
		logfk(ERROR, "PS2 controller failed self test! Resp: %d\n", status);
		this->enabled = false;
		this->port1_enabled = false;
		this->port2_enabled = false;
		return false;
	}

	logfk(KERNEL, "PS2 controller passed self test! Resp: %d\n", status);
	return true;
	
}

bool ps2_driver::input_ready() {
	uint8_t status = inportb(PS2_STATUS_PORT);
	//printfk("%x\n", status);
	if ((status & 0b00000010) == 0) {
		return true;
	}
	return false;
}

bool ps2_driver::output_ready() {
	uint8_t status = inportb(PS2_STATUS_PORT);
	//logfk(INFO, "ps2 status: %d\n", status);
	if ((status & 0b00000001) == 1) {
		return true;
	}
	return false;
}

void ps2_driver::disable_port1() {
	while(!this->input_ready()) {}
	outportb(PS2_CMD_PORT, 0xAD);
}

void ps2_driver::disable_port2() {
	while(!this->input_ready()) {}
	outportb(PS2_CMD_PORT, 0xA7);
}

void ps2_driver::enable_port1() {
	while(!this->input_ready()) {}
	outportb(PS2_CMD_PORT, 0xAE);
}

void ps2_driver::enable_port2() {
	while(!this->input_ready()) {}
	outportb(PS2_CMD_PORT, 0xA8);
}

void ps2_driver::test_port1() {
	while(!this->input_ready()) {}
	outportb(PS2_CMD_PORT, 0xAB);
	while(!this->output_ready()) {}
	uint8_t status = inportb(PS2_DATA_PORT);
	if (status == 0x00) {
		this->port1_enabled = true;
	} else if (status == 0x01) {
		logfk(ERROR, "PS2 Port1 failed self test: clock line stuck low\n");
		this->port1_enabled = false;
	} else if (status == 0x02) {
		logfk(ERROR, "PS2 Port1 failed self test: clock line stuck high\n");
		this->port1_enabled = false;
	} else if (status == 0x03) {
		logfk(ERROR, "PS2 Port1 failed self test: data line stuck low\n");
		this->port1_enabled = false;
	} else if (status == 0x04) {
		logfk(ERROR, "PS2 Port1 failed self test: data line stuck high\n");
		this->port1_enabled = false;
	}

}

void ps2_driver::test_port2() {
	while(!this->input_ready()) {}
	outportb(PS2_CMD_PORT, 0xA9);
	while(!this->output_ready()) {}
	uint8_t status = inportb(PS2_DATA_PORT);
	if (status == 0x00) {
		this->port2_enabled = true;
	} else if (status == 0x01) {
		logfk(ERROR, "PS2 Port2 failed self test: clock line stuck low\n");
		this->port2_enabled = false;
	} else if (status == 0x02) {
		logfk(ERROR, "PS2 Port2 failed self test: clock line stuck high\n");
		this->port2_enabled = false;
	} else if (status == 0x03) {
		logfk(ERROR, "PS2 Port2 failed self test: data line stuck low\n");
		this->port2_enabled = false;
	} else if (status == 0x04) {
		logfk(ERROR, "PS2 Port2 failed self test: data line stuck high\n");
		this->port2_enabled = false;
	}
}

void ps2_driver::flush_output_buffer() {
	//Ignore the value returned; we're just reading it to get junk outta the buffer
	inportb(PS2_DATA_PORT);
}

void ps2_driver::keyboard_irq_handler(struct registers* r) {
	//TODO: we're getting double characters now for some weird reason????
	uint8_t scancode = inportb(PS2_DATA_PORT);
	//Toggles shift key
	if (scancode == 0xAA || scancode == 0xB6) {
		this->shift_pressed = false;
		return;
	} else if (scancode == 0x36 || scancode == 0x2A) {
		this->shift_pressed = true;
		return;
	}
	
	// Toggle the right character set based upon shift
	uint8_t* chars = this->scancodes_set1;
	if (this->shift_pressed) chars = this->scancodes_set1_upper;

	// If we don't understand the scancode, just skip it
	if (chars[scancode] == 0) return;

	//Check top bit to see if key has been released
	if ((scancode & 0x80) != 0) return;

	Streams::stdin.append(chars[scancode]);
}

void ps2_driver::mouse_irq_handler(struct registers* r) {}
void ps2_driver::irq_handler(struct registers* r) {}
