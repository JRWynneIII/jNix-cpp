#pragma once
#include<vector.hpp>

// Position (64 bit) 	Value
// 0-3 	Magic number - 0x7F, then 'ELF' in ASCII
// 4 	1 = 32 bit, 2 = 64 bit
// 5 	1 = little endian, 2 = big endian
// 6 	ELF header version
// 7 	OS ABI - usually 0 for System V
// 8-15 	Unused/padding
// 16-17 	Type (1 = relocatable, 2 = executable, 3 = shared, 4 = core)
// 18-19 	Instruction set - see table below
// 20-23 	ELF Version (currently 1)
// 24-31 	Program entry offset
// 32-39 	Program header table offset
// 40-47 	Section header table offset
// 48-51 	Flags - architecture dependent; see note below
// 52-53 	ELF Header size
// 54-55 	Size of an entry in the program header table
// 56-57 	Number of entries in the program header table
// 58-59 	Size of an entry in the section header table
// 60-61 	Number of entries in the section header table
// 62-63 	Section index to the section header string table

typedef struct elf_header {
	uint32_t magic;
	uint8_t arch;
	uint8_t endianess;
	uint8_t header_version;
	uint8_t abi;
	uint64_t unused;
	uint16_t type;
	uint16_t instr_set;
	uint32_t elf_version;
	uint64_t entry_offset;
	uint64_t program_table_offset;
	uint64_t section_table_offset;
	uint32_t flags;
	uint16_t elf_header_size;
	uint16_t program_table_entry_size;
	uint16_t program_table_entry_count;
	uint16_t section_table_entry_size;
	uint16_t section_table_entry_count;
	uint16_t section_string_table_idx;
} elf_header_t;

//Position 	Value
//0-3 	Type of segment (see below)
//4-7 	Flags (see below)
//8-15 	The offset in the file that the data for this segment can be found (p_offset)
//16-23 	Where you should start to put this segment in virtual memory (p_vaddr)
//24-31 	Reserved for segment's physical address (p_paddr)
//32-39 	Size of the segment in the file (p_filesz)
//40-47 	Size of the segment in memory (p_memsz, at least as big as p_filesz)
//48-55 	The required alignment for this section (usually a power of 2) 
typedef struct elf_program_tbl_entry {
	uint32_t segment_type;
	uint32_t flags;
	uint64_t data_segment_offset;
	uint64_t segment_vaddr;
	uint64_t segment_paddr;
	uint64_t segment_size_in_file;
	uint64_t segment_size_in_memory;
	uint64_t alignment;
} elf_program_tbl_entry_t;

typedef struct elf_section_table_entry {
	uint32_t name_byte_offset;
	uint32_t type;
	uint64_t flags;
	uint64_t vaddr;
	uint64_t file_offset;
	uint64_t size;
	uint32_t link;
	uint32_t info;
	uint64_t alignment;
	uint64_t size_entries;
} elf_section_tbl_entry_t;

class Executable {
private:
	elf_header_t* header;
	uint8_t* file_data;
	char* path;
	int fd;
	elf_program_tbl_entry_t* program_table_ptr;
	elf_section_tbl_entry_t* section_table_ptr;
	uint8_t* string_table_ptr;
	vector<elf_section_tbl_entry_t*>* section_table;
	vector<elf_program_tbl_entry_t*>* program_table;

public:
	Executable() {}
	Executable(char* p);
	~Executable();
	void dump_header();
	void dump_program_table();
	void dump_section_table();
	void process_program_tbl();
	void process_section_tbl();
};
