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
	char* Name;
	Command() {}
	Command(char* s) : Name(s) {}
	//Command(String* s) : Name(*s) {}
	char* get_name() { return this->Name; }
	virtual void run() {
		logfk(INFO, "%s\n", Name);
	}
};

class TimerShowCommand : public Command {
public:
	TimerShowCommand() {}
	TimerShowCommand(char* s) : Command(s) {}
	//TimerShowCommand(String* s) : Command(*s) {}
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
	DeviceTreeCommand(char* s) : Command(s) {}
	//DeviceTreeCommand(String* s) : Command(*s) {}
	void run() {
		Devices::dump_device_tree();
	}
};

class SlabListCommand : public Command {
public:
	SlabListCommand() {}
	SlabListCommand(char* s) : Command(s) {}
	//SlabListCommand(String* s) : Command(*s) {}
	void run() {
		Memory::Paging::dump_slab_list();
	}
};

class LogListCommand : public Command {
public:
	LogListCommand() {}
	LogListCommand(char* s) : Command(s) {}
	//LogListCommand(String* s) : Command(*s) {}
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
	vector<char>* get_command() {
		vector<char>* ret = new vector<char>();
		char c = getch();
		while(c != '\n') {
			printfk("%c", c);
			ret->push_back(c);
			c = getch();
		}
		printfk("\n");
		return ret;
	}

	void start() {
		cmd_list().push_back(new Command("test"));
		cmd_list().push_back(new TimerShowCommand("timer"));
		cmd_list().push_back(new DeviceTreeCommand("devicetree"));
		cmd_list().push_back(new SlabListCommand("slablist"));
		cmd_list().push_back(new LogListCommand("loglist"));
		while(true) {
			printfk("(monitor) ");
			vector<char>* cmd = get_command();
			char* str = new char[cmd->length()];
			int idx=0;
			for ( auto c : *cmd ) { 
				str[idx] = c;
				idx++;
			}
		//	//TODO: fix this; shouldn't have to check length here. I think operator== is busted
			if (cmd->length() > 0) {
				for (auto c : cmd_list() ) {
					if (strcmp(c->get_name(), str)) {
						c->run();
					}
				}
			}
			delete cmd;
			delete str;
		}
	}
}
