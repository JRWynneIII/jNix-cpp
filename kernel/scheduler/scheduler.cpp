#include<kernel/process/elf.hpp>
#include<kernel/process/process.hpp>
#include<kernel/vfs/descriptor.hpp>
#include<kernel/devices/device_api.hpp>
#include<kernel/drivers/pit.hpp>
#include<vector.hpp>

namespace Kernel {
	//ARBITRARY
	uint64_t pid_max = 4096;
	namespace Scheduler {
		//Basically a fancy bitmap of available pids
		vector<uint64_t>& pids() {
			static vector<uint64_t>* p = new vector<uint64_t>();
			return *p;
		}
		
		//Returns ALL processes
		vector<process_t*>& process_queue() {
			static vector<process_t*>* pqueue = new vector<process_t*>();
			return *pqueue;
		}

		// Number of ticks left until next process is swapped to
		uint64_t ticks_left_for_cur_proc = 0;

		// NOTE: ARBITRARY NUMBERS
		uint64_t high_pri_task_ticks = 1000;
		uint64_t norm_pri_task_ticks = 100;
		uint64_t low_pri_task_ticks = 10;

		//The currently executing process
		process_t* cur_process = nullptr;

		process_t* get_next_proc_in_queue() {
			uint64_t idx = 0;
			//Return the first proccess in the queue we don't have a currently running proccess
			//this could be null as well if queue is empty!
			if (cur_process == nullptr) {
				if (process_queue().length() > 0)
					return process_queue().at(0);
			}
			for (auto proc : process_queue()) {
				if (proc == cur_process) {
					auto ret = process_queue().at(idx+1);
					// If we don't have any other processes left, then just return the first process.
					// or we are at the end, wrap around
					if (ret != nullptr) return ret;
					return process_queue().at(0);
				}
				idx++;
			}
		}

		void swap_to_next_process() {
			//pause cur process if we have one
			if (cur_process != nullptr) cur_process->pause();
			//get next process if there is one
			process_t* next_proc = get_next_proc_in_queue();
			//Set currently executing process
			cur_process = next_proc;

			//If we still don't have a process in the queue, just skip
			if (cur_process != nullptr) {
				//Set the number of ticks left
				switch(next_proc->getPriority()) {
					case proc_priority_t::HIGH:
						ticks_left_for_cur_proc = high_pri_task_ticks;
						break;
					case proc_priority_t::NORM:
						ticks_left_for_cur_proc = norm_pri_task_ticks;
						break;
					case proc_priority_t::LOW:
						ticks_left_for_cur_proc = low_pri_task_ticks;
						break;
					default:
						//Default to normal timeslice
						ticks_left_for_cur_proc = norm_pri_task_ticks;
						break;
				}
				cur_process->resume();
			}
		}

		void sched_on_tick() {
			//printfk("Checking for process...\n");
			//TODO: this is launched by a tick interrupt. The int handler saves the stack ptr and registers
			//	so we just need to check here, and when the interrupt is cleared, it should resume the
			//	currently running process, right????
			if (ticks_left_for_cur_proc == 0) {
			//	printfk("swapping process...\n");
				swap_to_next_process();
			} else {
				ticks_left_for_cur_proc--;
				//current_process->resume();
			}
		}


		void init() {
			//Preallocate all our pids
			for (uint64_t i = 0; i < Kernel::pid_max; i++) 
				pids().push_back(0);
			//set the function that gets called when a timer interrupt happens
			Device* pit = Devices::get_device_by_path("pit.programmable_interrupt_timer.1");
			if (pit != nullptr) {
				logfk(KERNEL, "Adding scheduler on_tick() to timer interrupt\n");
				pit_driver* d = static_cast<pit_driver*>(pit->get_driver());
				d->on_tick(100, sched_on_tick);
			}
			// Setting the ticks left, just so we don't trigger the task swapper when we don't want to
			//ticks_left_for_cur_proc = low_pri_task_ticks;
		}

		//Returns High priority processes
		vector<process_t*> high_priority_queue() {
			vector<process_t*> tree = vector<process_t*>();
			for (auto proc : process_queue())
				if (proc->getPriority() == proc_priority_t::HIGH) tree.push_back(proc);
			return tree;
		}
		//Returns norm priority processes
		vector<process_t*> norm_priority_queue() {
			vector<process_t*> tree = vector<process_t*>();
			for (auto proc : process_queue())
				if (proc->getPriority() == proc_priority_t::NORM) tree.push_back(proc);
			return tree;
		}
		//Returns low priority processes
		vector<process_t*> low_priority_queue() {
			vector<process_t*> tree = vector<process_t*>();
			for (auto proc : process_queue())
				if (proc->getPriority() == proc_priority_t::LOW) tree.push_back(proc);
			return tree;
		}

		//Returns kernel space processes
		vector<process_t*> kernel_processes() {
			vector<process_t*> tree = vector<process_t*>();
			// Kernel processes are always high priority
			for (auto h : high_priority_queue()) if (h->isKernelProc()) tree.push_back(h);
			return tree;
		}
		//Returns kernel space processes
		vector<process_t*> user_processes() {
			vector<process_t*> tree = vector<process_t*>();
			for (auto h : process_queue()) if (!h->isKernelProc()) tree.push_back(h);
			return tree;
		}

		process_t* get_cur_high_process() {}
		process_t* get_low_high_process() {}
		process_t* get_norm_high_process() {}

		uint64_t get_available_pid() {
			for (uint64_t i = 0; i < pids().length(); i++) {
				uint64_t pid = pids().at(i);
				if (pid == 0) {
					pids().set(i, 1);
					return i;
				}
			}
		}

		void remove_pid(uint64_t pid) {
			//Remove pid from the bitmap
			pids().set(pid, 0);
		}

		void add_process(char* cli, char* exec_path, proc_priority_t priority, bool is_kernel_proc) {
			uint64_t pid = get_available_pid();
			executable_t* e = new executable_t(exec_path, !is_kernel_proc);
			process_t* proc = new process_t(pid, cli, e, Kernel::pid_max, priority, is_kernel_proc);
			process_queue().push_back(proc);
			proc->allocate_stack();
			proc->load();
			//Run then Pause process immediately so we can use ->resume()?
			proc->run();
			proc->pause();
		}

		process_t* get_proc_by_pid(uint64_t pid) {
			for (auto proc : process_queue()) {
				if (proc->getPID() == pid) return proc;
			}
		}

		//TODO: create an enum of standard ret codes and signals
		//	this probably depends on syscalls and such here....
		uint64_t kill(uint64_t sig) {
			//TODO: based upon how aggressive we want (sig), stop and remove from queue
		}

		void remove_process(process_t* proc) {
			// Stop process and deallocate everything
			proc->stop();
			proc->unload();
			proc->free_stack();

			process_queue().del_by_value(proc);
			remove_pid(proc->getPID());
		}

		void remove_process(uint64_t pid) {
			process_t* proc = get_proc_by_pid(pid);
			remove_process(proc);
		}
	}
}
