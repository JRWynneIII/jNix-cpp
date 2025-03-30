#include<climits>
#include<kernel/vfs/inode.hpp>
#include<kernel/vfs/vnode.hpp>
#include<kernel/vfs/vfs.hpp>
#include<kernel/vfs/descriptor.hpp>
#include<kernel/drivers/fs_driver.hpp>
#include<kernel/drivers/driver_api.hpp>
#include<kernel/initrd.hpp>
#include<kernel.h>
#include<string.h>


namespace VFS {
	VFS_t& vfs() {
		static VFS_t* v = new VFS_t();
		return *v;
	}

	vector<fs_ident_t*>& mountpoints() {
		static vector<fs_ident_t*>* m = new vector<fs_ident_t*>();
		return *m;
	}

	vector<file_descriptor_t*>& open_fds() {
		static vector<file_descriptor_t*>* f = new vector<file_descriptor_t*>();
		return *f;
	}

	file_descriptor_t* get_fd(int fd) {
		for (auto f : open_fds()) {
			if (f->get_id() == fd) return f;
		}
		return nullptr;
	}

	int64_t find_free_fd() {
		for (int i = 0; i < INT_MAX; i++) {
			if (get_fd(i) == nullptr) return i;
		}
		return -1;
	}

	file_descriptor_t* create_new_fd(vnode_t* vnode, uint64_t pid, int flags, int mode) {
		int64_t id = find_free_fd();
		if (id == -1) {
			logfk(ERROR, "Out of file descriptors!\n");
			halt();
		}

		file_descriptor_t* fd = new file_descriptor_t(id, pid, vnode, flags, mode);
		vnode->add_stream(fd);
		return fd;
	}

	int destroy_fd(file_descriptor_t* fd) {
		open_fds().del_by_value(fd);
		//TODO: This might be a double-free for fd; need to check vector implementation
		delete fd;
		return 0;
	}

	vnode_t* prepare_sysroot() {
		logfk(KERNEL, "Creating VFS entry for /\n");
		//Create the root inode, This will need to be filled in with data
		//when you attach a real filesystem
		inode_t* root_inode = new inode_t(nullptr,0,777,0,0,0,sizeof(inode_t),1,1,1,1,4096, IDIR);
		vnode_t* root_vnode = new vnode_t("/", root_inode);

		return root_vnode;
	}

	void attach(fs_ident_t* fs) {
		//TODO: keep track of multiple subtrees in the vfs tree per vnode, 
		// since one can mount *something* ontop of something else
		// otherwise this causes a memory leak
		// or just delete the subtree
		//logfk(KERNEL, "Mounting %s within VFS\n", fs.mountpoint->name);
		if (strcmp(fs->mountpoint->name, "/")) {
			//TODO: don't do this on normal mounts. If you unmount /, it will not restore the tree underneath
			delete vfs().root;
			vfs().root = fs->mountpoint;
		} else {
			vnode_t* mountpoint_vnode = lookup(fs->path);
			//TODO: don't do this on normal mounts. If you unmount /, it will not restore the tree underneath
			delete mountpoint_vnode;
			*mountpoint_vnode = *(fs->mountpoint);
		}
	}

	void mount(fs_ident_t* fs) {
		attach(fs);
		mountpoints().push_back(fs);
	}

	void mount(vnode_t* vnode, fs_driver_t* driver, char* path) {
		fs_ident_t* fs = new fs_ident_t;
		fs->driver = driver;
		fs->mountpoint = vnode;
		fs->path = path;
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

	//int create(char* path, int mode) {

	//}

	int open(char* path, int flags, int mode) {
		//PID of 0 == kernel
		vnode_t* vnode = stat(path);
		if (vnode == nullptr) return -1; //File not found
		file_descriptor_t* fd = create_new_fd(vnode, 0, flags, mode);
		open_fds().push_back(fd);
		return fd->get_id();
	}

	//TODO Implement errno (https://man7.org/linux/man-pages/man3/errno.3.html) and set errno to posix error codes
	//     Or, change these to return custom error codes
	int close(int fd) {
		file_descriptor_t* desc = get_fd(fd);
		if (desc == nullptr) return -1;
		return destroy_fd(desc);
	}

	off_t seek(int fd, uint64_t offset, int whence) {
		file_descriptor_t* desc = get_fd(fd);
		if (desc == nullptr) return (off_t)-1;


		switch(whence) {
			case SEEK_SET:
				if (offset > desc->get_vnode()->inode->size) return (off_t)-1;
				desc->set_position(offset);
				break;
			case SEEK_CUR:
				if (offset + desc->get_position() > desc->get_vnode()->inode->size) return (off_t)-1;
				desc->seek(offset);
				break;
			case SEEK_END:
				if (offset + desc->get_vnode()->inode->size > desc->get_vnode()->inode->size) return (off_t)-1;
				desc->set_position(desc->get_vnode()->inode->size + offset);
				break;
			default:
				return (off_t)-1;
				break;
		}

		return desc->get_position();
	}

	size_t write(int fd, void* buffer, size_t count) {
		file_descriptor_t* desc = get_fd(fd);
		if (desc == nullptr) return -1;

		inode_t* ino = desc->get_vnode()->inode;

		if (ino->type == IDIR) return 0;

		// get the mountpoint associated with the inode
		fs_ident_t* mp = ino->fs_ident;
		// call driver's read with the inode and mointpoint data
		
		auto ret = mp->driver->write(buffer, ino, desc->get_position(), count);
		//Advance the position of the fd `count` bytes
		auto sret = desc->seek(ret);

		return ret;
	}

	size_t read(int fd, void* buffer, size_t count) {
		file_descriptor_t* desc = get_fd(fd);
		if (desc == nullptr) return 0;

		inode_t* ino = desc->get_vnode()->inode;

		if (ino->type == IDIR) return 0;

		// get the mountpoint associated with the inode
		fs_ident_t* mp = ino->fs_ident;
		// call driver's read with the inode and mointpoint data
		
		auto ret = mp->driver->read(ino, buffer, count);
		//Advance the position of the fd `count` bytes
		desc->seek(ret);

		return ret;
	}

	//Internal function; not exposed to libc/etc
	size_t read(inode_t* ino, void* buffer, size_t count) {
		// get the mountpoint associated with the inode
		fs_ident_t* mp = ino->fs_ident;
		// call driver's read with the inode and mointpoint data
		return mp->driver->read(ino, buffer, count);
	}

	//TODO: probably need to add fscanf/fread/etc and such to libc/libk

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
