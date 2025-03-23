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
#include<kernel/time.hpp>
#include<kernel/vfs/vnode.hpp>
#include<kernel/vfs/vfs.hpp>

class StatCatCommand : public Command {
public:
	StatCatCommand() {}
	//LSCommand(String s) : Command(s) {}
	StatCatCommand(char* s) : Command(s) {}
	void run() {
		vnode_t* file = VFS::stat("/bin/cat");
		if (file != nullptr) {
			inode_t* ino = file->inode;
			printfk("%s: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
					file->name,
					ino->fs_ident, 
					ino->inode_num,
					ino->mode,
					ino->ctime,
					ino->mtime,
					ino->atime,
					ino->size,
					ino->uid,
					ino->gid,
					ino->nlinks,
					ino->blocks,
					ino->block_size
			);
			
		}
	}
};

class LSBinCommand : public Command {
public:
	LSBinCommand() {}
	//LSCommand(String s) : Command(s) {}
	LSBinCommand(char* s) : Command(s) {}
	void run() {
		vector<vnode_t*>* dir = VFS::readdir("/bin");
		if (dir != nullptr) {
			for (auto i : *dir) {
				printfk("%s\n", i->name);
			}
		}
	}
};

class LSCommand : public Command {
public:
	LSCommand() {}
	//LSCommand(String s) : Command(s) {}
	LSCommand(char* s) : Command(s) {}
	void run() {
		vector<vnode_t*>* dir = VFS::readdir("/");
		if (dir != nullptr) {
			for (auto i : *dir) {
				printfk("%s\n", i->name);
			}
		}
	}
};

class ClockShowCommand : public Command {
public:
	ClockShowCommand() {}
	//ClockShowCommand(String s) : Command(s) {}
	ClockShowCommand(char* s) : Command(s) {}
	void run() {
		Device* clock = Devices::get_device_by_path("rtc.real_time_clock.1");
		if (clock != nullptr) {
			rtc_driver* driver = static_cast<rtc_driver*>(clock->get_driver());
			printfk("%d %d/%d %d:%d:%d\n",
					driver->get_year(),
					driver->get_month(),
					driver->get_day(),
					driver->get_hours(),
					driver->get_minutes(),
					driver->get_seconds());
		} else {
			logfk(ERROR, "Could not find timer\n");
		}
	}
};

class SleepTestCommand : public Command {
public:
	SleepTestCommand() {}
	//SleepTestCommand(String s) : Command(s) {}
	SleepTestCommand(char* s) : Command(s) {}
	void run() {
		Kernel::Time::sleep(1000);
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
		cmd_list().push_back(new SleepTestCommand("sleep"));
		cmd_list().push_back(new ClockShowCommand("clock"));
		cmd_list().push_back(new LSCommand("ls"));
		cmd_list().push_back(new LSBinCommand("lbin"));
		cmd_list().push_back(new StatCatCommand("stat"));
		cmd_list().push_back(new HelpCommand("help"));
		while(true) {
			printfk("(monitor) ");
			String cmd = get_command();
			//TODO: Split cmd by ' ' and pass rest of string as arguments to run()
			for (auto c : cmd_list() ) {
				if (c->get_name() == cmd) {
					c->run();
				}
			}
		}
	}
}
