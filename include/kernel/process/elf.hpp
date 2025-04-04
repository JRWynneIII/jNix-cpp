#pragma once
#include<vector.hpp>

// 64 bit vs 32 bit
typedef enum struct elf_class : uint8_t {
	ELFCLASS32 = 1,
	ELFCLASS64 = 2
} elf_class_t;

typedef enum struct elf_encoding : uint8_t {
	ELFDATA2LSB = 1,
	ELFDATA2MSB = 2
} elf_encoding_t;

typedef enum struct elf_osabi : uint8_t {
	ELFOSABI_SYSV = 0,
	ELFOSABI_HPUX = 1,
	ELFOSABI_STANDALONE = 255
} elf_osabi_t;

typedef enum struct elf_type : uint16_t {
	ET_NONE = 0, 		//No file type
	ET_REL = 1, 		//Relocatable object file
	ET_EXEC = 2, 		//Executable file
	ET_DYN = 3, 		//Shared object file
	ET_CORE = 4, 		//Core file
	ET_LOOS = 0xFE00, 	//Environment-specific use
	ET_HIOS = 0xFEFF,
	ET_LOPROC = 0xFF00, 	//Processor-specific use
	ET_HIPROC = 0xFFFF
} elf_type_t;

typedef struct elf_header {
	uint32_t magic;
	elf_class_t arch;
	elf_encoding_t endianess;
	uint8_t header_version;
	elf_osabi abi;
	uint64_t unused;
	elf_type_t type;
	uint16_t instr_set;
	uint32_t elf_version;
	uint64_t entry_vaddr;
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

//Section types
typedef enum struct section_type : uint32_t {
	SHT_NULL = 0,
	SHT_PROGBITS = 1,
	SHT_SYMTAB = 2,
	SHT_STRTAB = 3,
	SHT_RELA = 4,
	SHT_HASH = 5,
	SHT_DYNAMIC = 6,
	SHT_NOTE = 7,
	SHT_NOBITS = 8,
	SHT_REL = 9,
	SHT_SHLIB = 10,
	SHT_DYNSYM = 11,
	SHT_LOOS = 0x60000000,
	SHT_HIOS = 0x6FFFFFFF,
	SHT_LOPROC = 0x70000000,
	SHT_HIPROC = 0x7FFFFFFF
} section_type_t;

//Section flags
typedef enum struct section_flag : uint64_t {
	SHF_WRITE = 0x1,
	SHF_ALLOC = 0x2,
	SHF_EXECINSTR = 0x4,
	SHF_MASKOS = 0x0F000000,
	SHF_MASKPROC = 0xF0000000
} section_flag_t;

//Special section indicies
typedef enum struct section_idx : uint32_t {
	SHN_UNDEF = 0,
	SHN_LOPROC = 0xFF00,
	SHN_HIPROC = 0xFF1F,
	SHN_LOOS = 0xFF20,
	SHN_HIOS = 0xFF3F,
	SHN_ABS = 0xFFF1,
	SHN_COMMON = 0xFFF2
} section_idx_t;

typedef struct elf_section_table_entry {
	uint32_t name_byte_offset;
	section_type_t type;
	section_flag_t flags;
	uint64_t vaddr;
	uint64_t file_offset;
	uint64_t size;
	section_type_t link;
	section_type_t info;
	uint64_t alignment;
	uint64_t size_entries;
} elf_section_tbl_entry_t;

//Symbol type
typedef enum struct symbol_type : uint8_t {
	STT_NOTYPE = 0,
	STT_OBJECT = 1,
	STT_FUNC = 2,
	STT_SECTION = 3,
	STT_FILE = 4,
	STT_LOOS = 10,
	STT_HIOS = 12,
	STT_LOPROC = 13,
	STT_HIPROC = 15
} symbol_type_t;

//Symbol binding
typedef enum struct symbol_binding : uint8_t {
	STB_LOCAL = 0,
	STB_GLOBAL = 1,
	STB_WEAK = 2,
	STB_LOOS = 10,
	STB_HIOS = 12,
	STB_LOPROC = 13,
	STB_HIPROC = 15
} symbol_binding_t;

typedef struct elf_symbol_tbl_entry {
	uint32_t name_byte_offset;
	// info contains:
	// lower 4 bits: symbol_type_t
	// higher 4 bits: symbol_binding_t
	uint8_t info;
	uint8_t reserved;
	uint16_t section_idx;
	uint64_t value;
	uint64_t size;
} elf_symbol_tbl_entry_t;

class Executable {
private:
	elf_header_t* header;
	uint8_t* file_data;
	char* path;
	int fd;

	elf_program_tbl_entry_t* program_table_ptr;
	elf_section_tbl_entry_t* section_table_ptr;
	elf_symbol_tbl_entry_t* symbol_table_ptr;

	uint64_t num_symbol_tbl_entries;
	uint8_t* section_string_table_ptr;
	uint8_t* symbol_string_table_ptr;

	vector<elf_section_tbl_entry_t*>* section_table;
	vector<elf_program_tbl_entry_t*>* program_table;
	vector<elf_symbol_tbl_entry_t*>* symbol_table;

public:
	Executable() {}
	Executable(char* p);
	~Executable();
	void dump_header();
	void dump_program_table();
	void dump_section_table();
	void dump_symbol_table();
	void process_program_tbl();
	void process_section_tbl();
	void process_symbol_tbl();
	void load();
	void unload();
	void kill(uint64_t sig);
	void execute(); // Or add to scheduler? idk yet
	char* enum_to_str(section_type_t e);
	char* enum_to_str(symbol_binding_t e);
	char* enum_to_str(symbol_type_t e);
	char* enum_to_str(section_flag_t e);
	char* enum_to_str(section_idx_t e);
	char* enum_to_str(elf_class_t e);
	char* enum_to_str(elf_encoding_t e);
	char* enum_to_str(elf_osabi_t e);
	char* enum_to_str(elf_type_t e);
};
