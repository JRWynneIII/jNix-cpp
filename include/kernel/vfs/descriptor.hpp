#pragma once
#include<cstdint>
#include<kernel/vfs/vnode.hpp>

class file_descriptor_t {
private:
	int fd;
	uint64_t position;
	uint64_t process_id;
	int flags;
	int mode;
	vnode_t* vnode;
public:
//	file_descriptor_t(uint64_t f) : fd(f) {}
//	file_descriptor_t(uint64_t f, vnode_t* v) : fd(f), vnode(v) {}
//	file_descriptor_t(uint64_t f, uint64_t pid) : fd(f), process_id(pid) {}
	file_descriptor_t(int f, uint64_t pid, vnode_t* v, int fl, int m) : fd(f), process_id(pid), vnode(v), flags(fl), mode(m) {}
	~file_descriptor_t() {}
	int seek(int64_t bytes) {
		if (this->position + bytes > vnode->inode->size) {
			this->position = vnode->inode->size;
			return -1;
		} else if (this->position + bytes > UINT64_MAX) {
			return -2;
		} if ((int64_t)(this->position) + bytes < 0) {
			this->position = 0;
			return -1;
		}
		this->position += bytes;
	};
	uint64_t get_position() { return this->position; }
	void set_position(uint64_t p) { this->position = p; }
	uint64_t get_pid() { return this->process_id; }
	void set_pid(uint64_t p) { this->process_id = p; }
	uint64_t get_id() { return this->fd; }
	void set_vnode(vnode_t* v) { this->vnode = v; }
	vnode_t* get_vnode() { return this->vnode; }
	
};
