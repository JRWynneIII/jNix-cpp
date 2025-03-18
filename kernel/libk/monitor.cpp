#include<cstdint>
#include<cstddef>
#include<stdint.h>
#include<stddef.h>
#include<stdlib.h>
#include<cxxabi.h>
#include<kernel.h>
#include<kernel/memory.h>
#include<kernel/gdt.h>
#include<kernel/interrupts.h>
#include<kernel/drivers/driver_api.hpp>
#include<kernel/devices/device_api.hpp>
#include<string.hpp>

class Command {
public:
	String Name;
	Command() {}
	//Command(String s) : Name(s) {}
	Command(char* s) : Name(String(s)) {}
	String get_name() { return this->Name; }
	virtual void run() {
		logfk(INFO, "%s\n", Name.cstring());
	}
};

class TimerShowCommand : public Command {
public:
	TimerShowCommand() {}
	//TimerShowCommand(String s) : Command(s) {}
	TimerShowCommand(char* s) : Command(s) {}
	void run() {
		Device* timer = Devices::get_device_by_path("pit.programmable_interrupt_timer.1");
		if (timer != nullptr) {
			pit_driver* driver = static_cast<pit_driver*>(timer->get_driver());
			printfk("Cur ticks: %d\n", driver->get_ticks());
		} else {
			logfk(ERROR, "Could not find timer\n");
		}
	}
};

class DeviceTreeCommand : public Command {
public:
	DeviceTreeCommand() {}
	//DeviceTreeCommand(String s) : Command(s) {}
	DeviceTreeCommand(char* s) : Command(s) {}
	void run() {
		Devices::dump_device_tree();
	}
};

class SlabListCommand : public Command {
public:
	SlabListCommand() {}
	//SlabListCommand(String s) : Command(s) {}
	SlabListCommand(char* s) : Command(s) {}
	void run() {
		Memory::Paging::dump_slab_list();
	}
};

class LogListCommand : public Command {
public:
	LogListCommand() {}
	//LogListCommand(String s) : Command(s) {}
	LogListCommand(char* s) : Command(s) {}
	void run() {
		int idx = 0;
		//Fake pagination
		for (auto i : Kernel::kernel_logs()) {
			printk(i.cstring());
			printk("\n");
			if (idx >= 50) {
				//Wait for a key press
				getch();
				idx = 0;
			}
			idx++;
		}
	}
};

namespace Monitor {
	vector<Command*>& cmd_list() {
		static vector<Command*>* cmdlist = new vector<Command*>();
		return *cmdlist;
	}
	//TODO: replace this with cin/kin
	String get_command() {
		String ret = String();
		char c = getch();
		while(c != '\n') {
			printfk("%c", c);
			ret += c;
			c = getch();
		}
		printfk("\n");
		return ret;
	}

	void start() {
		//String* cmds = new String[5];
		Command* cmds[5]; // = new String[5];
		cmds[0] = new Command("test");
		cmds[1] = new TimerShowCommand("timer");
		cmds[2] = new DeviceTreeCommand("devicetree");
		cmds[3] = new SlabListCommand("slablist");
		cmds[4] = new LogListCommand("loglist");
		cmd_list().push_back(cmds[0]);
		cmd_list().push_back(cmds[1]);
		cmd_list().push_back(cmds[2]);
		cmd_list().push_back(cmds[3]);
		cmd_list().push_back(cmds[4]);
		while(true) {
			printfk("(monitor) ");
			String cmd = get_command();
		//	//TODO: fix this; shouldn't have to check length here. I think operator== is busted
			if (cmd.length() > 0) {
				for (auto c : cmd_list() ) {
					if (c->get_name() == cmd) {
						c->run();
					}
				}
			}
		}
	}
}
