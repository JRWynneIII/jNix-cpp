#pragma once
#include<kernel/process/elf.hpp>
#include<kernel/vfs/descriptor.hpp>
#include<kernel/memory.h>
#include<vector.hpp>

typedef enum proc_priority {
	HIGH,
	NORM,
	LOW
} proc_priority_t;

typedef struct context {
	uint64_t rax;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rbx;
	uint64_t rsp;
	uint64_t rbp;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t rip;
	uint64_t r8;
	uint64_t r9;
	uint64_t r10;
	uint64_t r11;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	uint64_t rflags;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t es;
	uint32_t fs;
	uint32_t gs;
} context_t;

typedef struct tss {
	uint16_t iopb;
	uint16_t reserved0; 	// 	0x64
	uint32_t reserved1; 	//	0x60
	uint32_t reserved2;	//	0x5c
	uint32_t ist7_hi; 	// 	0x58
	uint32_t ist7_lo; 	// 	0x54
	uint32_t ist6_hi; 	// 	0x50
	uint32_t ist6_lo; 	// 	0x4c
	uint32_t ist5_hi; 	// 	0x48
	uint32_t ist5_lo; 	// 	0x44
	uint32_t ist4_hi; 	// 	0x40
	uint32_t ist4_lo; 	// 	0x3c
	uint32_t ist3_hi; 	// 	0x38
	uint32_t ist3_lo; 	// 	0x34
	uint32_t ist2_hi; 	// 	0x30
	uint32_t ist2_lo; 	// 	0x2c
	uint32_t ist1_hi; 	// 	0x28
	uint32_t ist1_lo; 	// 	0x24
	uint32_t reserved3; 	//	0x20
	uint32_t reserved4; 	//	0x1c
	uint32_t rsp2_hi; 	// 	0x18
	uint32_t rsp2_lo; 	// 	0x14
	uint32_t rsp1_hi; 	// 	0x10
	uint32_t rsp1_lo; 	// 	0x0c
	uint32_t rsp0_hi; 	// 	0x08
	uint32_t rsp0_lo; 	// 	0x04
	uint32_t reserved5; 	//	0x00
} tss_t;

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

	context_t* context;
	bool is_kernel_proc;
	proc_priority_t priority;
	vector<file_descriptor_t*>* fds;
	uint64_t fd_max; //Max number of open files allowed
	tss_t* tss;
	uint64_t cpuid;
	uint64_t pid;
	Executable* executable;
	char* cmdline;
	vector<char*>* stdin;
	vector<char*>* stderr;
	vector<char*>* stdout;
	process_state_t state;
	//vector<process_t*> children;
public:
	process_t(uint64_t id, char* cli, Executable* exec, uint64_t fmax, proc_priority_t pri, bool iskernelproc);
	~process_t();

	//Getters
	vector<char*>* getStdin();
	vector<char*>* getStderr();
	vector<char*>* getStdout();
	Executable* getExecutable();
	char* getCmdline();
	process_state_t get_state();
	tss_t* getTSS();
	context_t* getContext();
	bool isKernelProc();
	proc_priority_t getPriority();
	uint64_t getPID();

	//Setters
	void setExecutable(Executable* e);
	void setCmdline(char* c);
	void set_state(process_state_t s);
	void setTSS(tss_t* t);
	void setContext(context_t* c);
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
