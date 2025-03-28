#include<kernel/vfs/inode.hpp>
#include<kernel/vfs/vnode.hpp>
#include<kernel/vfs/vfs.hpp>
#include<kernel/drivers/fs_driver.hpp>
#include<kernel/drivers/driver_api.hpp>
#include<kernel/initrd.hpp>
#include<kernel.h>
#include<string.h>

typedef struct fs_ident {
	fs_driver_t* driver;
	vnode_t* mountpoint;
	char* path;
} fs_ident_t;

namespace VFS {
	VFS_t& vfs() {
		static VFS_t* v = new VFS_t();
		return *v;
	}

	vector<fs_ident_t>& mountpoints() {
		static vector<fs_ident_t>* m = new vector<fs_ident_t>();
		return *m;
	}

	vnode_t* prepare_sysroot() {
		logfk(KERNEL, "Creating VFS entry for /\n");
		//Create the root inode, This will need to be filled in with data
		//when you attach a real filesystem
		inode_t* root_inode = new inode_t(0,0,777,0,0,0,sizeof(inode_t),1,1,1,1,4096, IDIR);
		vnode_t* root_vnode = new vnode_t("/", root_inode);

		return root_vnode;
	}

	void attach(fs_ident_t fs) {
		//TODO: keep track of multiple subtrees in the vfs tree per vnode, 
		// since one can mount *something* ontop of something else
		// otherwise this causes a memory leak
		// or just delete the subtree
		//logfk(KERNEL, "Mounting %s within VFS\n", fs.mountpoint->name);
		if (strcmp(fs.mountpoint->name, "/")) {
			//TODO: don't do this on normal mounts. If you unmount /, it will not restore the tree underneath
			delete vfs().root;
			vfs().root = fs.mountpoint;
		} else {
			vnode_t* mountpoint_vnode = lookup(fs.path);
			//TODO: don't do this on normal mounts. If you unmount /, it will not restore the tree underneath
			delete mountpoint_vnode;
			*mountpoint_vnode = *(fs.mountpoint);
		}
	}

	void mount(vnode_t* vnode, fs_driver_t* driver, char* path) {
		fs_ident_t fs = {
			driver,
			vnode,
			path
		};
		attach(fs);
		mountpoints().push_back(fs);
	}

	void init() {
		//Create our '/' vnode and attach it
		vnode_t* root = prepare_sysroot();
		mount(root, Initrd::driver, "/");
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
				char* cur_tok = new char[size+1];
				for(int i = 0; i < size; i++) cur_tok[i] = path[idx+i];
				cur_tok[size] = '\0';
				tokens->push_back(cur_tok);
				idx += (size-1);
			} else {
				cur++;
			}
			idx++;
		}
		return tokens;
	}

	vector<vnode_t*>* readdir(char* path) {
		vnode_t* dir = stat(path);
		if (dir != nullptr) return dir->get_children();
		return nullptr;
	}

	//TODO: add versions of readdir/lookup/stat/etc for inode number

	vnode_t* lookup(char* path) {
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
				logfk(ERROR, "VFS: lookup/stat() failed on path %s\n", path);
				return;
			}
			cur = child;
		}
		if (cur != nullptr) return cur;
		return nullptr;
	}

	vnode_t* find(vnode_t* root, char* path) {
		if (path[0] != '/') {
			logfk(ERROR, "VFS: Invalid path: %s\n", path);
			return;
		}

		vector<char*>* spath = split_path(path);

		vnode_t* cur = root;

		if (strcmp(path, "/"))
			return cur;

		for (auto dir : *spath) {
			vnode_t* child = nullptr;

			for ( auto c : *(cur->get_children())) {
				if (strcmp(c->name, dir)) { child = c; break; }
			}
			
			if (child == nullptr) {
				logfk(ERROR, "VFS: find() failed on path %s\n", path);
				return;
			}
			cur = child;
		}
		if (cur != nullptr) return cur;
		return nullptr;
	}

	vnode_t* stat(char* path) {
		return lookup(path);
	}
}
