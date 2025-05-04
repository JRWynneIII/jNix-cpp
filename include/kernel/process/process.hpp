#pragma once
#include<kernel/process/elf.hpp>
#include<kernel/vfs/descriptor.hpp>
#include<kernel/memory.h>
#include<vector.hpp>
#include<kernel/tss.hpp>

typedef enum proc_priority {
	HIGH,
	NORM,
	LOW
} proc_priority_t;

typedef enum process_state {
	RUNNING,
	SLEEPING,
	IOWAIT,
	PAUSED,
	DEFUNCT,
	STARTING,
	STOPPING
} process_state_t;

class process_t {
private:
	//Page table structure
	pml4_dir_t* pml4;
	pdp_dir_t* pdp;
	pd_dir_t* pd;
	pt_dir_t* pt;

	cpu_context_t* context;
	bool is_kernel_proc;
	proc_priority_t priority;
	vector<file_descriptor_t*>* fds;
	uint64_t fd_max; //Max number of open files allowed
	tss_t* tss;
	uint64_t cpuid;
	uint64_t pid;
	executable_t* executable;
	char* cmdline;
	vector<char*>* stdin;
	vector<char*>* stderr;
	vector<char*>* stdout;
	process_state_t state;
	//vector<process_t*> children;
public:
	process_t(uint64_t id, char* cli, executable_t* exec, uint64_t fmax, proc_priority_t pri, bool iskernelproc);
	~process_t();

	//Getters
	vector<char*>* getStdin();
	vector<char*>* getStderr();
	vector<char*>* getStdout();
	executable_t* getExecutable();
	char* getCmdline();
	process_state_t get_state();
	tss_t* getTSS();
	cpu_context_t* getContext();
	bool isKernelProc();
	proc_priority_t getPriority();
	uint64_t getPID();

	//Setters
	void setExecutable(executable_t* e);
	void setCmdline(char* c);
	void set_state(process_state_t s);
	void setTSS(tss_t* t);
	void setContext(cpu_context_t* c);
	void setPriority(proc_priority_t p);

	//allocates 0'd slab in kernel space, and modifies the process's page map to map it to a virtual addr
	uintptr_t calloc(uint64_t bytes, bool readonly, bool executable);

	void allocate_stack();
	void load();
	void setup_proc_page_table();
	void run();

	void pause();
	void resume();

	void stop();
	void unload();
	void free_stack();
	void destroy_proc_page_table();
};
