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
	Command(String s) : Name(s) {}
	String get_name() { return this->Name; }
	virtual void run() {
		logfk(INFO, "%s\n", Name.cstring());
	}
};

class TimerShowCommand : public Command {
public:
	TimerShowCommand() {}
	TimerShowCommand(String s) : Command(s) {}
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
	DeviceTreeCommand(String s) : Command(s) {}
	void run() {
		Devices::dump_device_tree();
	}
};

class SlabListCommand : public Command {
public:
	SlabListCommand() {}
	SlabListCommand(String s) : Command(s) {}
	void run() {
		Memory::Paging::dump_slab_list();
	}
};

class LogListCommand : public Command {
public:
	LogListCommand() {}
	LogListCommand(String s) : Command(s) {}
	void run() {
		int idx = 0;
		//Fake pagination
		for (auto i : Kernel::kernel_logs()) {
			printfk("%s\n", i.cstring());
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
		String ret;
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
		cmd_list().push_back(new Command(String("test")));
		cmd_list().push_back(new TimerShowCommand(String("timer")));
		cmd_list().push_back(new DeviceTreeCommand(String("devicetree")));
		cmd_list().push_back(new SlabListCommand(String("slablist")));
		cmd_list().push_back(new LogListCommand(String("loglist")));
		while(true) {
			printfk("(monitor) ");
			String cmd = get_command();
			for (auto c : cmd_list() ) {
				if (c->get_name() == cmd) {
					c->run();
				}
			}
		}
	}
}
