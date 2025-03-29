#pragma once
#include<kernel/vfs/inode.hpp>
#include<vector.hpp>
// Represents one discrete object in the VFS. Contains ptrs to it's children and 
// its parents and "itself" (inode). A symlink will have a separate vnode entry that 
// points to the same inode

class file_descriptor_t;

class vnode_t {
private:
	vnode_t* parent;
	vector<vnode_t*>* children;
	vector<file_descriptor_t*>* streams;
public:
	char* name;
	inode_t* inode;
	vnode_t(char* n, inode_t* i) : name(n), inode(i) {
		this->children = new vector<vnode_t*>();
		this->streams = new vector<file_descriptor_t*>();
	}

	~vnode_t() {
		//TODO: You might need to relink the children nodes to 
		//a new parent before you delete this node
		//TODO: delete all children and children's children, etc
		delete this->children;
	}

	vnode_t* get_parent() { return this->parent; }
	vector<vnode_t*>* get_children() { return this->children; }

	void set_parent(vnode_t* p) { this->parent = p; }
	void set_children(vector<vnode_t*>* c) { this->children = c; }
	void add_child(vnode_t* c) { this->children->push_back(c); }
	void add_stream(file_descriptor_t* f) { this->streams->push_back(f); }
	void remove_stream(file_descriptor_t* f) { this->streams->del_by_value(f); }
	uint64_t num_open_streams() { return this->streams->length(); }
};

