#include<cstdint>
#include<cstddef>
#include<stdint.h>
#include<stddef.h>
#include<stdlib.h>
#include<cxxabi.h>
#include<kernel.h>
#include<string.h>
#include<kernel/memory.h>
#include<kernel/gdt.h>
#include<kernel/interrupts.h>
#include<kernel/drivers/driver_api.hpp>
#include<kernel/devices/device_api.hpp>
#include<kernel/monitor.hpp>
#include<kernel/monitor/command.hpp>
#include<kernel/time.hpp>
#include<kernel/vfs/vnode.hpp>
#include<kernel/vfs/vfs.hpp>

class StatCommand : public Command {
public:
	StatCommand() {}
	StatCommand(char* s) : Command(s) {}
	void run(vector<char*>* args) {
		if (args->length() <= 0) {
			logfk(ERROR, "No path provided for stat\n");
			return;
		}
		vnode_t* file = VFS::stat(args->pop_head());
		if (file != nullptr) {
			inode_t* ino = file->inode;
			printfk("%s:\n\tfs_ident: %d,\n\tinode: %d,\n\tmode: %d,\n\tctime: %d,\n\tmtime: %d,\n\tatime: %d,\n\tsize: %d,\n\tuid: %d,\n\tgid: %d,\n\tnlinks: %d,\n\tblocks: %d,\n\tblock_size: %d\n",
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

class StatCatCommand : public Command {
public:
	StatCatCommand() {}
	StatCatCommand(char* s) : Command(s) {}
	void run(vector<char*>* args) {
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
	LSBinCommand(char* s) : Command(s) {}
	void run(vector<char*>* args) {
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
	LSCommand(char* s) : Command(s) {}
	void run(vector<char*>* args) {
		if (args->length() <= 0) {
			logfk(ERROR, "No path provided for ls\n");
			return;
		}
		char* path = args->pop_head();
		vector<vnode_t*>* dir = VFS::readdir(path);
		if (dir != nullptr) {
			for (auto i : *dir) {
				printfk("%s\n", i->name);
			}
		}
	}
};

class LSRootCommand : public Command {
public:
	LSRootCommand() {}
	LSRootCommand(char* s) : Command(s) {}
	void run(vector<char*>* args) {
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
	ClockShowCommand(char* s) : Command(s) {}
	void run(vector<char*>* args) {
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
	SleepTestCommand(char* s) : Command(s) {}
	void run(vector<char*>* args) {
		Kernel::Time::sleep(1000);
	}
};

class TimerShowCommand : public Command {
public:
	TimerShowCommand() {}
	TimerShowCommand(char* s) : Command(s) {}
	void run(vector<char*>* args) {
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
	void run(vector<char*>* args) {
		Devices::dump_device_tree();
	}
};

class SlabListCommand : public Command {
public:
	SlabListCommand() {}
	SlabListCommand(char* s) : Command(s) {}
	void run(vector<char*>* args) {
		Memory::Paging::dump_slab_list();
	}
};

class InterruptTestCommand : public Command {
public:
	InterruptTestCommand() {}
	InterruptTestCommand(char* s) : Command(s) {}
	void run(vector<char*>* args) {
		Interrupts::test();
	}
};

class PagingTestCommand : public Command {
public:
	PagingTestCommand() {}
	PagingTestCommand(char* s) : Command(s) {}
	void run(vector<char*>* args) {
		Memory::Paging::test();
	}
};

class HelpCommand : public Command {
public:
	HelpCommand() {}
	HelpCommand(char* s) : Command(s) {}
	void run(vector<char*>* args) {
		for ( auto i : Monitor::cmd_list() ) {
			printfk("%s\n", i->get_name());
		}
	}
};

namespace Monitor {
	vector<Command*>& cmd_list() {
		static vector<Command*>* c = new vector<Command*>();
		return *c;
	}

	vector<char*>* strsplit(char* path, char delim) {
		vector<char*>* tokens = new vector<char*>();
		int idx = 0;
		char* cur = path;

		while(*cur != '\0') {
			int size = 0;

			while (*cur != delim && *cur != '\0') {
				size++;
				cur++;
			}
			if (size > 0) {
				char* cur_tok = new char[size+1];
				for(int i = 0; i < size; i++) cur_tok[i] = path[idx+i];
				cur_tok[size] = '\0';
				tokens->push_back(cur_tok);
				idx += size;
			}
			cur++;
			idx++;
		}

		return tokens;
	}
	char* get_command() {
		vector<char>* vec = new vector<char>();
		char c = getch();
		while(c != '\n') {
			printfk("%c", c);
			//TODO: push_back causing a memory leak/overwriting bounds?
			vec->push_back(c);
			c = getch();
		}

		printfk("\n");
		char* ret = new char[vec->length()+1];
		int idx = 0;

		for (char i : *vec) {
			ret[idx] = i;
			idx++;
		}

		ret[idx++] = '\0';
		delete vec;
		return ret;
	}

	void start() {
		cmd_list().push_back(new Command("test"));
		cmd_list().push_back(new TimerShowCommand("timer"));
		cmd_list().push_back(new DeviceTreeCommand("devicetree"));
		cmd_list().push_back(new SlabListCommand("slablist"));

		cmd_list().push_back(new InterruptTestCommand("inttest"));
		cmd_list().push_back(new PagingTestCommand("pagetest"));
		cmd_list().push_back(new SleepTestCommand("sleep"));
		cmd_list().push_back(new ClockShowCommand("clock"));
		cmd_list().push_back(new LSCommand("ls"));
		cmd_list().push_back(new LSRootCommand("lsroot"));
		cmd_list().push_back(new LSBinCommand("lbin"));
		cmd_list().push_back(new StatCatCommand("statcat"));
		cmd_list().push_back(new StatCommand("stat"));
		cmd_list().push_back(new HelpCommand("help"));

		while(true) {
			printfk("(monitor) ");
			//char* cmd = get_command();
			char* cmd = get_command();
			//Split cmd by ' ' and pass rest of string as arguments to run()
			vector<char*>* split_cmd = strsplit(cmd, ' ');
			char* command = split_cmd->pop_head();
			for (auto c : cmd_list() ) {
				if (strcmp(c->get_name(), command)) {
					c->run(split_cmd);
				}
			}
			delete cmd;
		}
	}
}
