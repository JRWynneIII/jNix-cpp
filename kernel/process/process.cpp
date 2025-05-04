#include<kernel/process/elf.hpp>
#include<kernel/process/process.hpp>
#include<kernel/vfs/descriptor.hpp>
#include<kernel/memory.h>
#include<string.h>
#include<vector.hpp>

process_t::process_t(uint64_t id, char* cli, executable_t* exec, uint64_t fmax, proc_priority_t pri, bool iskernelproc) : pid(id), cmdline(cli), executable(exec), fd_max(fmax), priority(pri), is_kernel_proc(iskernelproc) {
	this->state = STARTING;
	this->stdin = new vector<char*>();
	this->stdout = new vector<char*>();
	this->stderr = new vector<char*>();
	this->fds = new vector<file_descriptor_t*>();
	//Probably shouldn't have this on the stack i'd bet
	this->tss = new tss_t;
	//populate tss
	this->context = new cpu_context_t{0}; //Zeroing out the registers
}

process_t::~process_t() {
	this->state = STOPPING;
	delete this->stdin;
	delete this->stdout;
	delete this->stderr;
	delete this->executable;
	delete this->fds;
	delete this->tss;
	delete this->context;
	delete this->pml4;
	delete this->pdp;
	delete this->pd;
	delete this->pt;
}

void process_t::setup_proc_page_table() {
	// Create a new page directory/pml4 structure thats mapped in kernel space (so.....malloc?)
	this->pml4 = new pml4_dir_t;
	this->pdp = new pdp_dir_t;
	this->pd = new pd_dir_t;
	this->pt = new pt_dir_t;
	// Get current page table structure
	uintptr_t cur_pml4_ptr;
	asm volatile("mov %%cr3, %0" : "=r"(cur_pml4_ptr));
	cur_pml4_ptr &= ~0xFFF; 
	pml4_dir_t* cur_pml4 = (pml4_dir_t*) TO_VIRT_ADDR(cur_pml4_ptr);
	// Copy kernel's pagetables to this new structure (so at this point any address thats mapped in the kernel, is 
	// mapped in the process.)
	// Since we copied the kernel's PML4, its already pointing to everything that the kernel has already allocated, 
	// So no need to copy any lower table
	memcpy(this->pml4->dir, cur_pml4_ptr, 512*sizeof(pml4_entry_t));
	// Set CR3 to the new directory?
	// we need to ensure that all allocations after swapping CR3, are for the process' virtual memory space ONLY
}

uintptr_t process_t::calloc(uint64_t bytes, bool readonly, bool executable) {
	//ualloc size, get ptr
	uintptr_t ptr = Memory::Allocation::uallocate(bytes, 1, readonly, executable, false);
	//zero out entry
	for (int i = 0; i < bytes; i++) reinterpret_cast<uint8_t*>(ptr)[i] = 0;
	//use ptr to add entry to this->pml4
	page_map_t map = {
		.pml4 = this->pml4,
		.pdp = this->pdp,
		.pd = this->pd,
		.pt = this->pt
	};
	
	//This will map the same virtual address in kernel space to user space....
	Memory::VMM::map_address(ptr, TO_PHYS_ADDR(ptr), readonly, executable, true, &Memory::VMM::kernel_address_space());
}

//Process preparation
void process_t::allocate_stack() {}

void process_t::load() {
	this->executable->load();
	// Make sure that the pages allocated
	// for the process are User space, and the appropriate r/w/x perms
}

//process_t state control
void process_t::run() {
	this->executable->execute();
	this->state = RUNNING;
}
void process_t::pause() {
	this->state = PAUSED;
}
void process_t::resume() {
	this->state = RUNNING;
}
void process_t::stop() {
	//TODO: Implement signals for kill?
	this->executable->kill(9);
	this->state = STOPPING;
}

//process_t cleanup
void process_t::unload() {
	this->executable->unload();
}
void process_t::free_stack() {}


//Getters/setters
uint64_t process_t::getPID() { return this->pid; }

void process_t::setPriority(proc_priority_t p) { this->priority = p; }

bool process_t::isKernelProc() { return this->is_kernel_proc; }

proc_priority_t process_t::getPriority() { return this->priority; }

tss_t* process_t::getTSS() { return this->tss; }

void process_t::setTSS(tss_t* t) { this->tss = t; }

void process_t::setContext(cpu_context_t* c) { this->context = c; }

cpu_context_t* process_t::getContext() { return this->context; }

vector<char*>* process_t::getStdin() { return this->stdin; }

vector<char*>* process_t::getStderr() { return this->stderr; }

vector<char*>* process_t::getStdout() { return this->stdout; }

void process_t::setExecutable(executable_t* e) { this->executable = e; }

void process_t::setCmdline(char* c) { this->cmdline = c; }

executable_t* process_t::getExecutable() { return this->executable; }

char* process_t::getCmdline() { return this->cmdline; }

void process_t::set_state(process_state_t s) { this->state = s; }

process_state_t process_t::get_state() { return this->state; }
