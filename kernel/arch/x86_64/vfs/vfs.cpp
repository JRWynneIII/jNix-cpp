#include<kernel/vfs/inode.hpp>
#include<kernel/vfs/vnode.hpp>
#include<kernel/vfs/vfs.hpp>
#include<kernel.h>

namespace VFS {
	VFS_t& vfs() {
		static VFS_t* v = new VFS_t();
		return *v;
	}

	vnode_t* pre_mount(char* path) {
		logfk(KERNEL, "Creating temporary sysroot\n");
		//Create the root inode, This will need to be filled in with data
		//when you attach a real filesystem
		inode_t* root_inode = new inode_t(0,0,777,0,0,0,sizeof(inode_t),1,1,1,1,4096, IDIR);
		inode_t* bin_inode = new inode_t(1,1,777,0,0,0,sizeof(inode_t),1,1,1,1,4096, IDIR);
		inode_t* test_file_inode = new inode_t(2,2,777,0,0,0,1356,1,1,1,1,4096, IFILE);
		inode_t* ls_inode = new inode_t(3,3,777,0,0,0,sizeof(inode_t),1,1,1,1,4096, IFILE);
		inode_t* cat_inode = new inode_t(4,4,777,0,0,0,sizeof(inode_t),1,1,1,1,4096, IFILE);

		vnode_t* root_vnode = new vnode_t(path, root_inode);
		vnode_t* bin_vnode = new vnode_t("bin", bin_inode);
		vnode_t* test_file_vnode = new vnode_t("test_file", test_file_inode);
		vnode_t* ls_vnode = new vnode_t("ls", ls_inode);
		vnode_t* cat_vnode = new vnode_t("cat", cat_inode);

		root_vnode->add_child(bin_vnode);
		root_vnode->add_child(test_file_vnode);

		bin_vnode->add_child(ls_vnode);
		bin_vnode->add_child(cat_vnode);

		return root_vnode;
	}

	void mount(vnode_t* root_vnode) {
		logfk(KERNEL, "Mounting %s within VFS\n", root_vnode->name);
		vfs().root = root_vnode;
	}

	void init() {
		vnode_t* root = pre_mount("/");
		mount(root);
	}

	vector<char*>* split_path(char* path) {
		vector<char*>* tokens = new vector<char*>();
		int idx = 0;
		char* cur = path;

		while(*cur != '\0') {
			int size = 0;

			while (*cur != '/' && *cur != '\0') {
				size++;
				cur++;
			}
			if (size > 0) {
				char* cur_tok = new char[size];
				for(int i = 0; i < size; i++) cur_tok[i] = path[idx+i];
				tokens->push_back(cur_tok);
				idx += size;
			}
			cur++;
			idx++;
		}
		return tokens;
	}

	vector<vnode_t*>* readdir(char* path) {
		vnode_t* dir = stat(path);
		if (dir != nullptr) return dir->get_children();
		return nullptr;
	}

	vnode_t* stat(char* path) {
		if (path[0] != '/') {
			logfk(ERROR, "VFS: Invalid path: %s\n", path);
			return;
		}

		vector<char*>* spath = split_path(path);

		vnode_t* cur = vfs().root;

		if (strcmp(path, "/"))
			return cur;

		for (auto dir : *spath) {
			vnode_t* child = nullptr;

			for ( auto c : *(cur->get_children())) {
				if (strcmp(c->name, dir)) { child = c; break; }
			}
			
			if (child == nullptr) {
				logfk(ERROR, "VFS: stat() failed on path %s\n", path);
				return;
			}
			cur = child;
		}
		if (cur != nullptr) return cur;
		return nullptr;
	}
}
