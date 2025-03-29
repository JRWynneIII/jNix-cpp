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
	uint64_t parse_size(uint8_t* input);
	initrd_header_t* read_header(uint64_t idx);
	virtual void install();
	virtual void write(uint8_t* data, uint64_t ino);
	virtual size_t read(inode_t* ino, void* buffer, size_t count);
	virtual void mount();
	virtual void irq_handler(struct registers* r);
	void dump_file_headers();
};
