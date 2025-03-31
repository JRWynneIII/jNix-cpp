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

	elf_section_tbl_entry_t* string_tbl_entry = this->section_table_ptr + this->header->section_string_table_idx;
	this->string_table_ptr = this->file_data + (string_tbl_entry->file_offset);

	this->program_table = new vector<elf_program_tbl_entry_t*>();
	this->section_table = new vector<elf_section_tbl_entry_t*>();

	this->process_program_tbl();
	this->process_section_tbl();
} 

void Executable::process_program_tbl() {
	elf_section_tbl_entry_t* string_tbl = this->section_table_ptr + this->header->section_string_table_idx;
	for (int i = 0 ; i < this->header->section_table_entry_count; i++) {
		elf_section_tbl_entry_t* cur = this->section_table_ptr + i;
		if (cur != string_tbl) {
			this->section_table->push_back(cur);
		}
	}
}
void Executable::process_section_tbl() {
	for (int i = 0 ; i < this->header->program_table_entry_count; i++) {
		elf_program_tbl_entry_t* cur = this->program_table_ptr + i;
		if (cur->segment_type < 5) this->program_table->push_back(cur);
	}
}

Executable::~Executable() {
	delete this->header;
	VFS::close(this->fd);
	delete this->program_table;
	delete this->section_table;
}

void Executable::dump_section_table() {
	printfk("Dumping Section Table\n");

	for (auto cur : *(this->section_table)) {
			char* name = this->string_table_ptr + cur->name_byte_offset;
			printfk("Section table entry:\n"
					"\tname: %s\n"
					"\tname_byte_offset: %x\n"
					"\ttype: %x\n"
					"\tflags: %x\n"
					"\tvaddr: %x\n"
					"\tfile_offset: %x\n"
					"\tsize: %x\n"
					"\tlink: %x\n"
					"\tinfo: %x\n"
					"\talignment: %x\n"
				        "\tsize_entries: %x\n",
				        name,
				        cur->name_byte_offset,
					cur->type,
					cur->flags,
					cur->vaddr,
					cur->file_offset,
					cur->size,
					cur->link,
					cur->info,
					cur->alignment,
					cur->size_entries);
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

void Executable::dump_header() {
	printfk("Dumping headers\n");
	if (this->header == nullptr) {
		logfk(ERROR, "ELF header is nullptr???\n");
		return;
	}
	printfk("%s:\n\tmagic: %x\n\tarch: %x\n\tendianess: %x\n\theader_version: %x\n\tabi: %x\n\tunused: %x\n\ttype: %x\n\tinstr_set: %x\n\telf_version: %x\n\tentry_offset: %x\n\tprogram_table_offset: %x\n\tsection_table_offset: %x\n\tflags: %x\n\telf_header_size: %x\n\tprogram_table_entry_size: %x\n\tprogram_table_entry_count: %x\n\tsection_table_entry_size: %x\n\tsection_table_entry_count: %x\n\tsection_string_table_idx: %x\n",
		this->path,
		this->header->magic,
		this->header->arch,
		this->header->endianess,
		this->header->header_version,
		this->header->abi,
		this->header->unused,
		this->header->type,
		this->header->instr_set,
		this->header->elf_version,
		this->header->entry_offset,
		this->header->program_table_offset,
		this->header->section_table_offset,
		this->header->flags,
		this->header->elf_header_size,
		this->header->program_table_entry_size,
		this->header->program_table_entry_count,
		this->header->section_table_entry_size,
		this->header->section_table_entry_count,
		this->header->section_string_table_idx);
}
