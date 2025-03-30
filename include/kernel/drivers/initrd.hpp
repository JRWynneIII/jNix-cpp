#pragma once
#include<kernel/drivers/driver.hpp>
#include<kernel/drivers/fs_driver.hpp>
#include<kernel/vfs/vnode.hpp>

typedef struct initrd_header {
	char name[100];
	uint8_t mode[8];
	uint8_t uid[8];
	uint8_t gid[8];
	uint8_t size[12];
	uint8_t mtime[12];
	uint8_t chksum[8];
	uint8_t typeflag[1];
	uint64_t inode_num;
	bool driver_allocated;
} initrd_header_t;

class initrd_driver : public fs_driver_t {
private:
	uintptr_t address;
	uint64_t size;
	vector<initrd_header_t*>* headers;
	vnode_t* root_vnode;
public:
	initrd_driver();
	~initrd_driver();

	virtual void install();
	virtual void remove(char* path);
	virtual void create(char* path, inode_t* ino);
	virtual size_t write(uint8_t* data, inode_t* ino, uint64_t offset, size_t bytes);
	virtual size_t read(inode_t* ino, void* buffer, size_t count);
	virtual void mount();
	virtual void irq_handler(struct registers* r);

	initrd_header_t* get_header_by_inode(inode_t* ino);
	initrd_header_t* read_header(uint64_t idx);
	uint64_t parse_size(uint8_t* input);
	void dump_file_headers();
	uint8_t* to_octal(uint64_t input);
};
