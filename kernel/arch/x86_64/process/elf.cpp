#include<kernel.h>
#include<string.h>
#include<kernel/process/elf.hpp>
#include<kernel/vfs/vfs.hpp>
#include<vector.hpp>


Executable::Executable(char* p) : path(p) {
	//TODO: set flags and have them mean something to open()
	int fd = VFS::open(p, 0, 0);
	uint64_t size = VFS::stat(p)->inode->size;

	uint8_t* file = new uint8_t[size];

	int rc = VFS::read(fd, file, size);
	if (rc != size) {
		logfk(ERROR, "Read for Executable %s failed with rc: %d", path, rc);
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

void Executable::process_section_tbl() {
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

void Executable::process_program_tbl() {
	for (int i = 0 ; i < this->header->program_table_entry_count; i++) {
		elf_program_tbl_entry_t* cur = this->program_table_ptr + i;
		this->program_table->push_back(cur);
	}
}

void Executable::process_symbol_tbl() {
	for (int i = 0; i < this->num_symbol_tbl_entries; i++) {
		elf_symbol_tbl_entry_t* cur = this->symbol_table_ptr + i;
		this->symbol_table->push_back(cur);
	}
}

Executable::~Executable() {
	delete this->header;
	VFS::close(this->fd);
	delete this->program_table;
	delete this->section_table;
	delete this->symbol_table;
}

void Executable::load() {}
void Executable::unload() {}
void Executable::kill(uint64_t sig) {}
void Executable::execute() {} // Or add to scheduler? idk yet

void Executable::dump_section_table() {
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

void Executable::dump_program_table() {
	for (auto cur : *(this->program_table)) {
		printfk("Program table entry:\n"
				"\tsegment_type: %x\n"
                                "\tflags: %x\n"
                                "\tdata_segment_offset: %x\n"
                                "\tsegment_vaddr: %x\n"
                                "\tsegment_paddr: %x\n"
                                "\tsegment_size_in_file: %x\n"
                                "\tsegment_size_in_memory: %x\n"
                                "\talignment: %x\n",
				cur->segment_type,
				cur->flags,
				cur->data_segment_offset,
				cur->segment_vaddr,
				cur->segment_paddr,
				cur->segment_size_in_file,
				cur->segment_size_in_memory,
				cur->alignment);
	}
}

void Executable::dump_symbol_table() {
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

void Executable::dump_header() {
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

char* Executable::enum_to_str(section_type_t e) {
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
char* Executable::enum_to_str(symbol_binding_t e) {
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
char* Executable::enum_to_str(symbol_type_t e) {
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
char* Executable::enum_to_str(section_flag_t e) {
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
char* Executable::enum_to_str(section_idx_t e) {
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
char* Executable::enum_to_str(elf_class_t e) {
	switch(e) {
		case elf_class_t::ELFCLASS32:
			return "ELFCLASS32";
		case elf_class_t::ELFCLASS64:
			return "ELFCLASS64";
	}
	return nullptr;
}
char* Executable::enum_to_str(elf_encoding_t e) {
	switch(e) {
		case elf_encoding_t::ELFDATA2LSB:
			return "ELFDATA2LSB";
		case elf_encoding_t::ELFDATA2MSB:
			return "ELFDATA2MSB";
	}
	return nullptr;
}
char* Executable::enum_to_str(elf_osabi_t e) {
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
char* Executable::enum_to_str(elf_type_t e) {
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
