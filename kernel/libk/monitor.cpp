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
#include<kernel/vfs/descriptor.hpp>
#include<kernel/process/elf.hpp>

class ReadELFCommand : public Command {
public:
	ReadELFCommand() {}
	ReadELFCommand(char* s) : Command(s) {}
	void run(vector<char*>* args) {
		Executable e = Executable(args->pop_head());
		e.dump_header();
		printfk("Press any key to continue\n");
		getch();
		e.dump_program_table();
		printfk("Press any key to continue\n");
		getch();
		e.dump_section_table();
	}
};

class ListHeadersCommand : public Command {
public:
	ListHeadersCommand() {}
	ListHeadersCommand(char* s) : Command(s) {}
	void run(vector<char*>* args) {
		auto mp = VFS::mountpoints().at(1);
		auto driver = static_cast<initrd_driver*>(mp->driver);
		driver->dump_file_headers();
	}
};
class ListFDCommand : public Command {
public:
	ListFDCommand() {}
	ListFDCommand(char* s) : Command(s) {}
	void run(vector<char*>* args) {
		for (auto i : VFS::open_fds()) {
			printfk("%s: %d\n", i->get_vnode()->name, i->get_id());
		}
	}
};
class WriteCommand : public Command {
public:
	WriteCommand() {}
	WriteCommand(char* s) : Command(s) {}
	void run(vector<char*>* args) {
		if (args->length() != 2) {
			logfk(ERROR, "write [path] [string to write]\n");
			return;
		}
		char* path = args->pop_head();
		char* byte_str = args->pop_head();
		size_t bytes = strlen(byte_str);

		//TODO: Flags and mode are not implemented yet
		int fd = VFS::open(path, 0, 0);

		if (fd >= 0) {
			//char* buffer = new char[bytes+1];
			size_t rc = VFS::write(fd, byte_str, bytes);
			if (rc == -1) {
				logfk(ERROR, "Write failed for: %s, %d\n", path, rc);
				return;
			}
			printfk("Wrote %d bytes to path: %s\n", rc, path);
			rc = VFS::close(fd);
			if (rc != 0) {
				logfk(ERROR, "Close failed for %s, rc: %d\n", path, rc);
				return;
			}
		} else {
			logfk(ERROR, "Write failed for: %s, fd: %d\n", path, fd);
		}
	}
};

class OpenCommand : public Command {
public:
	OpenCommand() {}
	OpenCommand(char* s) : Command(s) {}
	void run(vector<char*>* args) {
		if (args->length() != 2) {
			logfk(ERROR, "ropen [path] [bytes_to_read]\n");
			return;
		}
		char* path = args->pop_head();
		char* byte_str = args->pop_head();
		uint64_t bytes = atoi(byte_str);

		//TODO: Flags and mode are not implemented yet
		int fd = VFS::open(path, 0, 0);

		if (fd >= 0) {
			char* buffer = new char[bytes+1];
			size_t rc = VFS::read(fd, buffer, bytes);
			if (rc == -1) {
				logfk(ERROR, "Read failed for: %s, %d\n", path, rc);
				return;
			}
			//Since we don't have a libc that has functions to return EOF, lets just pretty it up
			buffer[rc] = '\0';
			printfk("%s\n", buffer);
			printfk("Read %d bytes from path: %s\n", rc, path);
			rc = VFS::close(fd);
			if (rc != 0) {
				logfk(ERROR, "Close failed for %s, rc: %d\n", path, rc);
				return;
			}
		} else {
			logfk(ERROR, "Open failed for: %s, fd: %d\n", path, fd);
		}
	}
};

class ReadCommand : public Command {
public:
	ReadCommand() {}
	ReadCommand(char* s) : Command(s) {}
	void run(vector<char*>* args) {
		if (args->length() != 2) {
			logfk(ERROR, "read [path] [bytes]\n");
			return;
		}
		char* path = args->pop_head();
		char* byte_str = args->pop_head();
		uint64_t bytes = atoi(byte_str);
		vnode_t* file = VFS::stat(path);

		if (file != nullptr) {
			char* buffer = new char[bytes+1];
			size_t rc = VFS::read(file->inode, buffer, bytes);
			buffer[rc] = '\0';
			printfk("%s\n", buffer);
			printfk("Read %d bytes from inode: %d\n", rc, file->inode->inode_num);
		} else {
			logfk(ERROR, "Read failed for: read %s, %d\n", path, bytes);
		}
	}
};

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
		cmd_list().push_back(new ReadCommand("read"));
		cmd_list().push_back(new WriteCommand("write"));
		cmd_list().push_back(new OpenCommand("ropen"));
		cmd_list().push_back(new ListFDCommand("listfd"));
		cmd_list().push_back(new ListHeadersCommand("listh"));
		cmd_list().push_back(new ReadELFCommand("readelfh"));
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
