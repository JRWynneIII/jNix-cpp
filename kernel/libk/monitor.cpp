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
#include<kernel/monitor.hpp>
#include<kernel/monitor/command.hpp>

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

class InterruptTestCommand : public Command {
public:
	InterruptTestCommand() {}
	//InterruptTestCommand(String s) : Command(s) {}
	InterruptTestCommand(char* s) : Command(s) {}
	void run() {
		Interrupts::test();
	}
};

class PagingTestCommand : public Command {
public:
	PagingTestCommand() {}
	//PagingTestCommand(String s) : Command(s) {}
	PagingTestCommand(char* s) : Command(s) {}
	void run() {
		Memory::Paging::test();
	}
};

class HelpCommand : public Command {
public:
	HelpCommand() {}
	//PagingTestCommand(String s) : Command(s) {}
	HelpCommand(char* s) : Command(s) {}
	void run() {
		for ( auto i : Monitor::cmd_list() ) {
			printfk("%s\n", i->get_name().cstring());
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
		cmd_list().push_back(new Command("test"));
		cmd_list().push_back(new TimerShowCommand("timer"));
		cmd_list().push_back(new DeviceTreeCommand("devicetree"));
		cmd_list().push_back(new SlabListCommand("slablist"));
		cmd_list().push_back(new LogListCommand("loglist"));

		cmd_list().push_back(new InterruptTestCommand("inttest"));
		cmd_list().push_back(new PagingTestCommand("pagetest"));
		cmd_list().push_back(new HelpCommand("help"));
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
