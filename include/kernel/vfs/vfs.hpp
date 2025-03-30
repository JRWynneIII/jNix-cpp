#pragma once
#include<kernel/vfs/inode.hpp>
#include<kernel/vfs/vnode.hpp>
#include<kernel/drivers/fs_driver.hpp>

enum seek_whence {
	SEEK_SET,
	SEEK_CUR,
	SEEK_END,
};

typedef enum seek_off_t {
	SEEK_DATA,
	SEEK_HOLE,
} off_t;

class VFS_t {
public:
	// This will be `/`. root.children is everything contained in /
	vnode_t* root;

	VFS_t() {}
	VFS_t(vnode_t* r) : root(r) {}
};

namespace VFS {
	VFS_t& vfs();
	vector<file_descriptor_t*>& open_fds();
	vector<fs_ident_t*>& mountpoints();
	vnode_t* pre_mount(char* path);
	vnode_t* find(vnode_t* root, char* path);
	void mount(vnode_t* vnode, fs_driver_t* driver, char* path);
	void init();
	vector<char*>* split_path(char* path);
	vector<vnode_t*>* readdir(char* path);
	vnode_t* stat(char* path);
	vnode_t* lookup(char* path);
	//char* get_path_for_inode(uint64_t inode);
	size_t read(inode_t* ino, void* buffer, size_t count);
	size_t read(int fd, void* buffer, size_t count);
	size_t write(int fd, void* buffer, size_t count);
	int open(char* path, int flags, int mode);
	int close(int fd);
	off_t seek(int fd, uint64_t offset, int whence);
}
