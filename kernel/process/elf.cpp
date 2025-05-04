#include<kernel.h>
#include<string.h>
#include<kernel/process/elf.hpp>
#include<kernel/vfs/vfs.hpp>
#include<kernel/panic.h>
#include<kernel/memory/addrspace.hpp>
#include<kernel/hardware.hpp>
#include<kernel/context.hpp>
#include<kernel/processor/x86_64.hpp>
#include<kernel/gdt.hpp>
#include<kernel/gdt.h>
#include<kernel/processor/x86_64.hpp>
#include<vector.hpp>


void executable_t::common_init() {
	//TODO: set flags and have them mean something to open()
	int fd = VFS::open(this->path, 0, 0);
	uint64_t size = VFS::stat(this->path)->inode->size;

	uint8_t* file = new uint8_t[size];

	int rc = VFS::read(fd, file, size);
	if (rc != size) {
		logfk(ERROR, "Read for executable %s failed with rc: %d", path, rc);
		this->fd = fd;
		this->header = (elf_header_t*)file;
		this->file_data = file;
		return;
	}

	this->fd = fd;
	this->header = (elf_header_t*)file;
	this->file_data = file;

	this->program_table_ptr = (elf_program_tbl_entry_t*)((uint8_t*)file + this->header->program_table_offset);
	this->section_table_ptr = (elf_section_tbl_entry_t*)((uint8_t*)file + this->header->section_table_offset);

	// If the section_string_table_ptr is undefined, then we don't have a section string table
	if ((section_idx_t)this->header->section_string_table_idx != section_idx_t::SHN_UNDEF) {
		elf_section_tbl_entry_t* string_tbl_entry = this->section_table_ptr + this->header->section_string_table_idx;
		this->section_string_table_ptr = this->file_data + (string_tbl_entry->file_offset);
	} else {
		this->section_string_table_ptr = nullptr;
	}

	this->program_table = new vector<elf_program_tbl_entry_t*>();
	this->section_table = new vector<elf_section_tbl_entry_t*>();
	this->symbol_table = new vector<elf_symbol_tbl_entry_t*>();

	this->process_program_tbl();
	this->process_section_tbl();
	this->process_symbol_tbl();
}
executable_t::executable_t(char* p) : path(p), is_userspace(false), argc(0) {
	this->argv = vector<char*>();
	this->common_init();
} 

executable_t::executable_t(char* p, bool user, uint64_t argc, vector<char*> argv) : path(p), is_userspace(user), argc(argc), argv(argv) {
	this->common_init();
}

executable_t::executable_t(char* p, bool user) : path(p), is_userspace(user), argc(0) {
	this->argv = vector<char*>();
	this->common_init();
} 

void executable_t::process_section_tbl() {
	for (int i = 0 ; i < this->header->section_table_entry_count; i++) {
		elf_section_tbl_entry_t* cur = this->section_table_ptr + i;
		this->section_table->push_back(cur);

		if (this->section_string_table_ptr == nullptr) {
			if (cur->type == section_type_t::SHT_STRTAB) 
				this->symbol_string_table_ptr = (uint8_t*)this->file_data + cur->file_offset;
		} else {
			char* name = this->section_string_table_ptr + cur->name_byte_offset;
			if (cur->type == section_type_t::SHT_STRTAB && strcmp(name, ".strtab"))
				this->symbol_string_table_ptr = (uint8_t*)this->file_data + cur->file_offset;
		} 

		if (cur->type == section_type_t::SHT_SYMTAB) {
			this->symbol_table_ptr = (elf_symbol_tbl_entry_t*)((uint8_t*)this->file_data + cur->file_offset);
			this->num_symbol_tbl_entries = (cur->size / sizeof(elf_symbol_tbl_entry_t));
			//Round up
			if (cur->size % sizeof(elf_symbol_tbl_entry_t) != 0) this->num_symbol_tbl_entries++;
		}
	}
}

void executable_t::process_program_tbl() {
	for (int i = 0 ; i < this->header->program_table_entry_count; i++) {
		elf_program_tbl_entry_t* cur = this->program_table_ptr + i;
		this->program_table->push_back(cur);
	}
}

void executable_t::process_symbol_tbl() {
	for (int i = 0; i < this->num_symbol_tbl_entries; i++) {
		elf_symbol_tbl_entry_t* cur = this->symbol_table_ptr + i;
		this->symbol_table->push_back(cur);
	}
}

executable_t::~executable_t() {
	delete this->header;
	VFS::close(this->fd);
	delete this->program_table;
	delete this->section_table;
	delete this->symbol_table;
}

bool executable_t::verify() {
	//Check to make sure it's 64 bit x86
	if (this->header->arch != elf_class_t::ELFCLASS64) return false;
	if (this->header->abi != elf_osabi_t::ELFOSABI_SYSV) return false;
	if (this->header->endianess != elf_encoding_t::ELFDATA2LSB) return false;
	if (this->header->elf_version != 1) return false;
	if (this->header->type != elf_type_t::ET_EXEC) return false;
	if (this->header->magic != 0x464C457F) return false;
	return true;
}

//Load segments into memory
void executable_t::load() {
	if (!this->verify()) {
		logfk(ERROR, "Invalid executable");
		return;
	}
	printfk("executable passed verification\n");
	this->context = {0};
	//Enables sysret/syscall
	enable_sysret();
	
	//Set up our address space
	this->addrspace = new addrspace_t(Memory::hhdm_offset, true);
	//Unsure if needed
	//TODO: calling bootstrap here is causing some bad addresses to be mapped i think
	this->addrspace->bootstrap();
	printfk("Address space initialized\n");

	//Get loadable segments
	vector<elf_program_tbl_entry_t*>* loadable_segments = new vector<elf_program_tbl_entry_t*>();

	//Find size of image and check for size mismatch
	this->image_size_bytes = 0;
	for (auto ph : *this->program_table) {
		if (ph->segment_type == elf_ptype_t::PT_LOAD) {
			loadable_segments->push_back(ph);
			if (ph->segment_size_in_file > ph->segment_size_in_memory) {
				logfk(ERROR, "Bad executable: Program header size mismatch!\n");
				return;
			}
			this->image_size_bytes += ph->segment_size_in_memory;
		}

	}

	//Add a guard page and 2 stack pages
	this->image_size_bytes += 4096 * 3;
	this->image_size_pages = this->image_size_bytes % 4096 == 0 ? this->image_size_bytes / 4096 : (this->image_size_bytes / 4096) + 1;
	printfk("Executable image is size %d\n", this->image_size_bytes);

	//Grab a contiguous chunk of memory for the process and map it in the kernel's addrspace
	this->image_addr = Memory::VMM::alloc_contiguous_pages(this->image_size_pages, true, false, this->is_userspace);
	printfk("Allocated %d pages for execuable image (%x)\n", this->image_size_pages, this->image_addr);

	//Grab the stack location
	this->ustack_frame = this->image_addr + (4096 * (this->image_size_pages - 2));
	this->kstack_frame = this->ustack_frame + 4096;

	//Set the guard page as inaccessible so if the process tries to write into the stack, it can't
	uintptr_t guard_page = this->ustack_frame - 4096;
	Memory::VMM::modify_page_perms(guard_page, true, false, false, this->addrspace);
	Memory::VMM::modify_page_perms(this->kstack_frame, true, false, false, this->addrspace);


	// So our image will look like this in memory 
	// |-----------segments-----------|--guard page--|--stack--|--kernel-stack--|
	//


	//Load segment into memory
	printfk("Loading executable pages into memory\n");
	uintptr_t cur_addr_in_image = this->image_addr;
	uintptr_t last_vaddr_mapped = 0x0;
	uint64_t last_segment_size = 0;
	for (auto ph : *loadable_segments) {
		//Check to make sure we're not writing into the stack area of the image
		if (cur_addr_in_image >= guard_page) {
			logfk(ERROR, "Reached end of segment section of executable image! This should not happen\n");
			return;
		}

		//Copy the segment into the image
		memcpy(cur_addr_in_image, this->file_data + ph->data_segment_offset, ph->segment_size_in_memory);
		//TODO: change the permissions on these segments based upon Pflag in ph!

		//Map the address into the processes address space
		for(uintptr_t vaddr = ph->segment_vaddr; vaddr < (ph->segment_vaddr + ph->segment_size_in_memory); vaddr += 4096) {
			this->addrspace->map_page(vaddr, TO_PHYS_ADDR(cur_addr_in_image), true, false, false, false, this->is_userspace);
			printfk("Mapped segment paddr %x to vaddr %x\n", TO_PHYS_ADDR(cur_addr_in_image), vaddr);
			//TODO: This might cause things to be misaligned in the image. We might want to change this to 
			//	add 4096 if cur_addr+4096 < (start_addr_in_image + ph->segment_size_in_memory)
			//	else, cur_addr = start_addr_in_image + ph->segment_size_in_memory
			cur_addr_in_image += 4096;
		}
		last_vaddr_mapped = ph->segment_vaddr;
		last_segment_size = ph->segment_size_in_memory;
		//cur_addr_in_image += ph->segment_size_in_memory;
	}

	//0 out the stacks
	memset(this->ustack_frame, 0, 4096);
	memset(this->kstack_frame, 0, 4096);
//	((uint64_t*)this->stack_frame)[0] = 0xffffffff;
//	((uint64_t*)this->stack_frame)[1] = this->argc;

	//Set rdi and rsi
	//rdi = argc, rsi = pointer to argv
	this->context.rdi = this->argc;
	this->context.rsi = 0x0;

	//Set the entry point and stack frame
	this->entrypoint = this->header->entry_vaddr;
	this->context.rip = this->entrypoint;

	//set rsp to our stack frames in the process image

	//Round up to next page boundary
	uintptr_t virt_guard_page_bottom = ((last_vaddr_mapped + last_segment_size) + 4096) & ~0xFFF;
	uintptr_t phys_guard_page_bottom = TO_PHYS_ADDR(guard_page);

	uintptr_t virt_user_stack_bottom = virt_guard_page_bottom + 4096;
	uintptr_t phys_user_stack_bottom = TO_PHYS_ADDR(this->ustack_frame);
	uintptr_t virt_user_stack_top = virt_user_stack_bottom + 4096;
	uintptr_t phys_user_stack_top = TO_PHYS_ADDR(this->ustack_frame + 4096);

	uintptr_t virt_kernel_stack_bottom = virt_user_stack_top;
	uintptr_t phys_kernel_stack_bottom = TO_PHYS_ADDR(this->kstack_frame);
	uintptr_t virt_kernel_stack_top = virt_user_stack_top + 4096;
	uintptr_t phys_kernel_stack_top = TO_PHYS_ADDR(this->kstack_frame + 4096);

	this->context.rsp = virt_user_stack_top - 8;
	
	//Map the stacks in the process's page map
	this->addrspace->map_page(virt_user_stack_bottom, phys_user_stack_bottom, true, false, false, false, this->is_userspace);
	//this->addrspace->map_page(user_proc_stack_top, user_phys_stack_bottom, true, false, false, false, this->is_userspace);
	this->addrspace->map_page(virt_kernel_stack_bottom, phys_kernel_stack_bottom, true, false, false, false, false);
	this->addrspace->map_page(virt_guard_page_bottom, phys_guard_page_bottom, true, false, false, false, false);

	printfk("Entrypoint is set to %x\n", this->entrypoint);
	printfk("Guard Page:\n\tProcess vaddr: %x\n\tKernel vaddr: %x\n\tPhysical addr: %x\n", virt_guard_page_bottom, guard_page, phys_guard_page_bottom);
	printfk("User Stack Page:\n\tProcess vaddr: %x\n\tKernel vaddr: %x\n\tPhysical addr: %x\n", virt_user_stack_bottom, this->ustack_frame, phys_user_stack_bottom);
	printfk("Kernel Stack Page:\n\tProcess vaddr: %x\n\tKernel vaddr: %x\n\tPhysical addr: %x\n", virt_kernel_stack_bottom, this->kstack_frame, phys_kernel_stack_bottom);
	printfk("Stack top/RSP set to: %x\n", virt_user_stack_top);
	printfk("Executable image load complete!\n");
}

void executable_t::unload() {}
void executable_t::kill(uint64_t sig) {}
// Or add to scheduler? idk yet
void executable_t::execute() {
	//Save our current context
	//TODO:Maybe contexts need to go in the Process or task object?
	save_cpu_context(&(this->kernel_context));
	uint64_t ring = 0;
	if (this->is_userspace) {
		ring = 3;
		this->context.cs = 0x1B;
		this->context.ss = 0x23;
	} else {
		this->context.cs = 0x8;
		this->context.ss = 0x10;
	}

	this->context.rflags = 0x202;

	this->addrspace->activate();
	//TODO: In ring 3 Interrupts appear to be triggering, which is good, but when swapping back to kernel, it page faults
	//	This is probably due to the fact SS isn't getting reset, so we're still on CPL=3, but CS's DPL=0
	//
	//	TSS should be causing the SS to get set to DPL0, but it isn't
	
	GDT::gdt().tss.rsp0 = Hardware::CPU().read_reg(x86_64_register::rsp);
	//TODO: Have a separate context switch for kernel tasks
	//	Right now, this fails since we move to ring 3 by default
	if (this->is_userspace)
		swap_to_context(&(this->context), ring);
	else
		swap_to_kernel_context(&(this->context), ring);
} __attribute__((noreturn)); 

void executable_t::dump_section_table() {
	printfk("Dumping Section Table\n");

	for (auto cur : *(this->section_table)) {
			printk("Section table entry:\n");
			if (cur->type != section_type_t::SHT_NULL) {
				char* type = this->enum_to_str(cur->type);
				char* flags = this->enum_to_str(cur->flags);
				char* link = this->enum_to_str(cur->link);
				char* info = this->enum_to_str(cur->info);
				if (this->section_string_table_ptr != nullptr) {
					char* name = this->section_string_table_ptr + cur->name_byte_offset;
					printfk("\tname: %s\n", name);
				}
				printfk("\ttype: %s\n"
					"\tflags: %s\n"
					"\tlink: %s\n"
					"\tinfo: %s\n",
					type,
					flags,
					link,
					info);
			}
	}
}

void executable_t::dump_program_table() {
	int count = 0;
	for (auto cur : *(this->program_table)) {
		if (count % 3) getch();
		count++;
		printfk("Program table entry:\n"
				"\tsegment_type: %s\n"
                                "\tflags: %s\n"
                                "\tdata_segment_offset: %x\n"
                                "\tsegment_vaddr: %x\n"
                                "\tsegment_paddr: %x\n"
                                "\tsegment_size_in_file: %x\n"
                                "\tsegment_size_in_memory: %x\n"
                                "\talignment: %x\n",
				this->enum_to_str(cur->segment_type),
				this->enum_to_str(cur->flags),
				cur->data_segment_offset,
				cur->segment_vaddr,
				cur->segment_paddr,
				cur->segment_size_in_file,
				cur->segment_size_in_memory,
				cur->alignment);
	}
}

void executable_t::dump_symbol_table() {
	int idx = 0;
	for (auto cur : *(this->symbol_table)) {
		//First idx is a null entry
		if (idx > 0) {
			char* name = this->symbol_string_table_ptr + cur->name_byte_offset;
			printfk("Symbol table entry:\n"
				"\tname: %s\n"
				"\tname_byte_offset: %d\n"
				"\tinfo: %x\n"
				"\tsection_idx: %d\n"
				"\tvalue: %x\n"
				"\tsize: %d\n",
				name,
				cur->name_byte_offset,
				cur->info,
				cur->section_idx,
				cur->value,
				cur->size);
		}
		idx++;
	}
}

void executable_t::dump_header() {
	printfk("Dumping headers\n");
	if (this->header == nullptr) {
		logfk(ERROR, "ELF header is nullptr???\n");
		return;
	}
	printfk("ELF Header:\n"
		"\tpath: %s\n"
		"\tmagic: %x\n"
		"\tarch: %s\n"
		"\tendianess: %s\n"
		"\theader_version: %d\n"
		"\tabi: %s\n"
		"\ttype: %s\n"
		"\tinstr_set: %x\n"
		"\telf_version: %d\n"
		"\tentry_vaddr: %x\n"
		"\tflags: %x\n",
		this->path,
		this->header->magic,
		this->enum_to_str(this->header->arch),
		this->enum_to_str(this->header->endianess),
		this->header->header_version,
		this->enum_to_str(this->header->abi),
		this->enum_to_str(this->header->type),
		this->header->instr_set,
		this->header->elf_version,
		this->header->entry_vaddr,
		this->header->flags);
}

char* executable_t::enum_to_str(section_type_t e) {
	switch(e) {
		case section_type_t::SHT_NULL:
			return "SHT_NULL";
		case section_type_t::SHT_PROGBITS:
			return "SHT_PROGBITS";
		case section_type_t::SHT_SYMTAB:
			return "SHT_SYMTAB";
		case section_type_t::SHT_STRTAB:
			return "SHT_STRTAB";
		case section_type_t::SHT_RELA:
			return "SHT_RELA";
		case section_type_t::SHT_HASH:
			return "SHT_HASH";
		case section_type_t::SHT_DYNAMIC:
			return "SHT_DYNAMIC";
		case section_type_t::SHT_NOTE:
			return "SHT_NOTE";
		case section_type_t::SHT_NOBITS:
			return "SHT_NOBITS";
		case section_type_t::SHT_REL:
			return "SHT_REL";
		case section_type_t::SHT_SHLIB:
			return "SHT_SHLIB";
		case section_type_t::SHT_DYNSYM:
			return "SHT_DYNSYM";
		case section_type_t::SHT_LOOS:
			return "SHT_LOOS";
		case section_type_t::SHT_HIOS:
			return "SHT_HIOS";
		case section_type_t::SHT_LOPROC:
			return "SHT_LOPROC";
		case section_type_t::SHT_HIPROC:
			return "SHT_HIPROC";
	}
	return nullptr;
}
char* executable_t::enum_to_str(symbol_binding_t e) {
	switch(e) {
		case symbol_binding_t::STB_LOCAL:
			return "STB_LOCAL";
		case symbol_binding_t::STB_GLOBAL:
			return "STB_GLOBAL";
		case symbol_binding_t::STB_WEAK:
			return "STB_WEAK";
		case symbol_binding_t::STB_LOOS:
			return "STB_LOOS";
		case symbol_binding_t::STB_HIOS:
			return "STB_HIOS";
		case symbol_binding_t::STB_LOPROC:
			return "STB_LOPROC";
		case symbol_binding_t::STB_HIPROC:
			return "STB_HIPROC";
	}
	return nullptr;
}
char* executable_t::enum_to_str(symbol_type_t e) {
	switch(e) {
		case symbol_type_t::STT_NOTYPE:
			return "STT_NOTYPE";
		case symbol_type_t::STT_OBJECT:
			return "STT_OBJECT";
		case symbol_type_t::STT_FUNC:
			return "STT_FUNC";
		case symbol_type_t::STT_SECTION:
			return "STT_SECTION";
		case symbol_type_t::STT_FILE:
			return "STT_FILE";
		case symbol_type_t::STT_LOOS:
			return "STT_LOOS";
		case symbol_type_t::STT_HIOS:
			return "STT_HIOS";
		case symbol_type_t::STT_LOPROC:
			return "STT_LOPROC";
		case symbol_type_t::STT_HIPROC:
			return "STT_HIPROC";
	}
	return nullptr;
}
char* executable_t::enum_to_str(section_flag_t e) {
	switch(e) {
		case section_flag_t::SHF_WRITE:
			return "SHF_WRITE";
		case section_flag_t::SHF_ALLOC:
			return "SHF_ALLOC";
		case section_flag_t::SHF_EXECINSTR:
			return "SHF_EXECINSTR";
		case section_flag_t::SHF_MASKOS:
			return "SHF_MASKOS";
		case section_flag_t::SHF_MASKPROC:
			return "SHF_MASKPROC";
	}
	return nullptr;
}
char* executable_t::enum_to_str(section_idx_t e) {
	switch(e) {
		case section_idx_t::SHN_UNDEF:
			return "SHN_UNDEF";
		case section_idx_t::SHN_LOPROC:
			return "SHN_LOPROC";
		case section_idx_t::SHN_HIPROC:
			return "SHN_HIPROC";
		case section_idx_t::SHN_LOOS:
			return "SHN_LOOS";
		case section_idx_t::SHN_HIOS:
			return "SHN_HIOS";
		case section_idx_t::SHN_ABS:
			return "SHN_ABS";
		case section_idx_t::SHN_COMMON:
			return "SHN_COMMON";
	}
	return nullptr;
}
char* executable_t::enum_to_str(elf_class_t e) {
	switch(e) {
		case elf_class_t::ELFCLASS32:
			return "ELFCLASS32";
		case elf_class_t::ELFCLASS64:
			return "ELFCLASS64";
	}
	return nullptr;
}
char* executable_t::enum_to_str(elf_encoding_t e) {
	switch(e) {
		case elf_encoding_t::ELFDATA2LSB:
			return "ELFDATA2LSB";
		case elf_encoding_t::ELFDATA2MSB:
			return "ELFDATA2MSB";
	}
	return nullptr;
}
char* executable_t::enum_to_str(elf_osabi_t e) {
	switch(e) {
		case elf_osabi_t::ELFOSABI_SYSV:
			return "ELFOSABI_SYSV";
		case elf_osabi_t::ELFOSABI_HPUX:
			return "ELFOSABI_HPUX";
		case elf_osabi_t::ELFOSABI_STANDALONE:
			return "ELFOSABI_STANDALONE";
	}
	return nullptr;
}
char* executable_t::enum_to_str(elf_type_t e) {
	switch(e) {
		case elf_type_t::ET_NONE:
			return "ET_NONE";
		case elf_type_t::ET_REL:
			return "ET_REL";
		case elf_type_t::ET_EXEC:
			return "ET_EXEC";
		case elf_type_t::ET_DYN:
			return "ET_DYN";
		case elf_type_t::ET_CORE:
			return "ET_CORE";
		case elf_type_t::ET_LOOS:
			return "ET_LOOS";
		case elf_type_t::ET_HIOS:
			return "ET_HIOS";
		case elf_type_t::ET_LOPROC:
			return "ET_LOPROC";
		case elf_type_t::ET_HIPROC:
			return "ET_HIPROC";
	}
	return nullptr;
}

char* executable_t::enum_to_str(elf_ptype_t e) {
	switch(e) {
		case elf_ptype_t::PT_NULL:
			return "PT_NULL";
		case elf_ptype_t::PT_LOAD:
			return "PT_LOAD";
		case elf_ptype_t::PT_DYNAMIC:
			return "PT_DYNAMIC";
		case elf_ptype_t::PT_INTERP:
			return "PT_INTERP";
		case elf_ptype_t::PT_NOTE:
			return "PT_NOTE";
		case elf_ptype_t::PT_SHLIB:
			return "PT_SHLIB";
		case elf_ptype_t::PT_PHDR:
			return "PT_PHDR";
		case elf_ptype_t::PT_LOOS:
			return "PT_LOOS";
		case elf_ptype_t::PT_HIOS:
			return "PT_HIOS";
		case elf_ptype_t::PT_LOPROC:
			return "PT_LOPROC";
		case elf_ptype_t::PT_HIPROC:
			return "PT_HIPROC";
	}
	return nullptr;
}

char* executable_t::enum_to_str(elf_pflag_t e) {
	switch(e) {
		case elf_pflag_t::PF_X:
			return "PF_X";
		case elf_pflag_t::PF_W:
			return "PF_W";
		case elf_pflag_t::PF_R:
			return "PF_R";
		case elf_pflag_t::PF_MASKOS:
			return "PF_MASKOS";
		case elf_pflag_t::PF_MASKPROC:
			return "PF_MASKPROC";
	}
	return nullptr;
}
