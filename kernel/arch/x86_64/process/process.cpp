#include<kernel/process/elf.hpp>
#include<kernel/process/process.hpp>
#include<kernel/vfs/descriptor.hpp>
#include<vector.hpp>

process_t::process_t(uint64_t id, char* cli, Executable* exec, uint64_t fmax, proc_priority_t pri, bool iskernelproc) : pid(id), cmdline(cli), executable(exec), fd_max(fmax), priority(pri), is_kernel_proc(iskernelproc) {
	this->state = STARTING;
	this->stdin = new vector<char*>();
	this->stdout = new vector<char*>();
	this->stderr = new vector<char*>();
	this->fds = new vector<file_descriptor_t*>();
	//Probably shouldn't have this on the stack i'd bet
	this->tss = new tss_t;
	//populate tss
	this->context = new context_t{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //Zeroing out the registers
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
}

uint64_t process_t::getPID() { return this->pid; }

void process_t::setPriority(proc_priority_t p) { this->priority = p; }

bool process_t::isKernelProc() { return this->is_kernel_proc; }

proc_priority_t process_t::getPriority() { return this->priority; }

tss_t* process_t::getTSS() { return this->tss; }

void process_t::setTSS(tss_t* t) { this->tss = t; }

void process_t::setContext(context_t* c) { this->context = c; }

context_t* process_t::getContext() { return this->context; }

vector<char*>* process_t::getStdin() { return this->stdin; }

vector<char*>* process_t::getStderr() { return this->stderr; }

vector<char*>* process_t::getStdout() { return this->stdout; }

void process_t::setExecutable(Executable* e) { this->executable = e; }

void process_t::setCmdline(char* c) { this->cmdline = c; }

Executable* process_t::getExecutable() { return this->executable; }

char* process_t::getCmdline() { return this->cmdline; }

void process_t::set_state(process_state_t s) { this->state = s; }

process_state_t process_t::get_state() { return this->state; }

//process_t preparation
void process_t::allocate_stack() {}

void process_t::load() {
	this->executable->load();
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

