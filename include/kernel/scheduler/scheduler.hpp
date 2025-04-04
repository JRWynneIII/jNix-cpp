#pragma once
#include<kernel/process/process.hpp>

namespace Kernel {
	namespace Scheduler {
		extern uint64_t pid_max;
		void init();
		vector<process_t*>& process_queue();
		void add_proccess(char* cli, char* exec_path, proc_priority_t priority, bool is_kernel_proc);
		void remove_process(process_t* proc);
		void remove_process(uint64_t pid);
	}
}
