#pragma once
#include<kernel/vfs/inode.hpp>
#include<kernel/vfs/vnode.hpp>

//Probably won't need this but might be nice to have
//class VFSIterator {
//private:
//	vnode_t* cur;
//public:
//	VFSIterator(vnode_t* i) : cur(i) {}
//
//	vnode_t& operator*() { return *cur; }
//
//	VFSIterator& operator++() {
//		this->cur = this->cur->get_next();
//		return *this;
//	}
//
//	bool operator!=(const VFSIterator& rhs) const { 
//		return this->cur != rhs.cur; 
//	}
//};

class VFS_t {
public:
	// This will be `/`. root.children is everything contained in /
	vnode_t* root;

	//PROBABLY not needed but won't work really. How do you do this with a graph?
//	VFSIterator begin() { return VFSIterator(root); }
//	VFSIterator end() { return VFSIterator(nullptr); }

	VFS_t() {}
	VFS_t(vnode_t* r) : root(r) {}
};

namespace VFS {
	VFS_t& vfs();
	vnode_t* pre_mount(char* path);
	void mount(vnode_t* root_vnode);
	void init();
	vector<char*>* split_path(char* path);
	vector<vnode_t*>* readdir(char* path);
	vnode_t* stat(char* path);
}
