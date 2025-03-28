#include<kernel.h>
#include<string.h>
#include<limine.h>
#include<kernel/drivers/driver.hpp>
#include<kernel/drivers/initrd.hpp>
#include<kernel/vfs/vnode.hpp>
#include<kernel/vfs/vfs.hpp>

namespace {
	__attribute__((used, section(".limine_requests")))
	static volatile struct limine_module_request module_req = {
	    .id = LIMINE_MODULE_REQUEST,
	    .revision = 0
	};
}

initrd_driver::initrd_driver() {
	this->enabled = false;
	this->desc = FILESYSTEM_DRIVER;
	this->set_name("initrd");
	this->headers = new vector<initrd_header_t*>();
}

initrd_driver::~initrd_driver() {
	//VFS::unmount(this->root_vnode);
	delete this->root_vnode;
	delete this->headers;
}

void initrd_driver::install() {
	if (module_req.response != nullptr) {
		for (uint64_t i = 0 ; i < module_req.response->module_count; i++) {
			struct limine_file* mod = module_req.response->modules[i];
			//logfk(INFO, "Module %d path: %s\n", i, mod->path);
			//logfk(INFO, "Module %d size: %d\n", i, mod->size);
			//logfk(INFO, "Module %d cmdline %s\n", i, mod->cmdline);
			if (strcmp(mod->cmdline, "initrd")) {
				logfk(INFO, "Found initrd; proceeding with install and mount...\n");
				this->address = mod->address;
				this->size = mod->size;
				break;
			}
		}
		this->enabled = true;
	} else {
		logfk(ERROR, "Invalid response from limine: module_req\n");
	}
}

void initrd_driver::write(uint8_t* data, uint64_t ino) {
}

uint8_t* initrd_driver::read(uint64_t ino) {
}

uint64_t initrd_driver::parse_size(uint8_t* input) {
	uint64_t size = 0;
	uint64_t count = 1;
	for (uint64_t i = 11; i > 0 ; i--, count *= 8) size += ((input[i-1] - '0') * count);
	return size;
}

initrd_header_t* initrd_driver::read_header(uint64_t idx) {
	uintptr_t cur_addr = this->address;
	uint64_t cur_idx = 0;
	initrd_header* cur = (initrd_header*)cur_addr;
	while (cur_idx != idx) {
		uint64_t size = this->parse_size(cur->size);
		cur_addr += (( size / 512) + 1) * 512;
		if (size % 512) cur_addr += 512;
		cur = (initrd_header*)cur_addr;
		cur_idx++;
	}

	if (cur->name[0] == '\0') return nullptr;
	return cur;

}

void initrd_driver::dump_file_headers() {
	logfk(KERNEL, "Dumping headers\n");
	for (auto header : *(this->headers)) {
		printfk("Found file: %s size: %d\n", header->name, this->parse_size(header->size));
	}
}

void initrd_driver::mount() {
	uint64_t idx = 0;
	initrd_header_t* cur = read_header(idx);
	idx++;

	while(cur != nullptr) {
		this->headers->push_back(cur);
		cur = read_header(idx);
		idx++;
	}

	//Create '/' vnode. This will be our VFS's root vnode that everything is built off of. 
	// This vnode will replace the "/" vnode in the VFS. 
	inode_t* root_inode = new inode_t(0,0,777,0,0,0,sizeof(inode_t),1,1,1,1,4096, IDIR);
	vnode_t* root_vnode = new vnode_t("/", root_inode);
	uint64_t inode_num = 1;

	for (auto header : *(this->headers)) {
		uint64_t size = this->parse_size(header->size);
		bool isdir = (size > 0) ? false : true;

		char* full_path = header->name; //strcat("/", header->name);
		uint64_t path_len = strlen(full_path);
		inode_type_t ino_type = IFILE;
		if (isdir) {
			//Truncate the last / off the path name if its a directory
			full_path[path_len - 1] = '\0';
			//Set as a directory inode
			ino_type = IDIR;
		}

		//Use VFS::split_path because this makes an easy way to get just the 'filename' or dir name
		//since TAR uses a "fully qualified" path (kinda) as the 'file name' in the header
		vector<char*>* split_name = VFS::split_path(full_path);
		char* vnode_name = split_name->at(split_name->length() - 1);
		delete split_name;

		//Create new inode and vnode objects for our VFS
		inode_t* ino = new inode_t(
				0, //Should always be the first index in vfs::mountpoints
				inode_num,
				atoi(header->mode),
				0,
				atoi(header->mtime),
				0,
				size,
				atoi(header->uid),
				atoi(header->gid),
				1,
				(size % 512) == 0 ? (size/512) : (size/512) + 1,
				512,
				ino_type);

		vnode_t* vn = new vnode_t(vnode_name, ino);

		//Find the full path of the parent
		char* parent_path = new char[strlen(full_path)+1];
		memcpy(parent_path, full_path, strlen(full_path) + 1);
		int idx = 0;
		int last_found_slash_idx = 0;
		while(parent_path[idx] != '\0') {
			if (parent_path[idx] == '/') last_found_slash_idx = idx;
			idx++;
		}

		//Truncate the path
		parent_path[last_found_slash_idx] = '\0';
		char* truncated_parent_path = strcat("/", parent_path);
		delete parent_path;

		//Lookup parent vnode
		vnode_t* parent = VFS::find(root_vnode, truncated_parent_path);
		parent->add_child(vn);
		delete truncated_parent_path;

		inode_num++;
	}

	VFS::mount(root_vnode, this, "/");
}

void initrd_driver::irq_handler(struct registers* r) {
}
