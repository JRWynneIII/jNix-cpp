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

uint8_t* initrd_driver::to_octal(uint64_t input) {
	uint8_t* buffer = new uint8_t[22]; //Max number of 'characters' for a uint64_t
	//Zero out the memory
	for (int i = 0 ; i < 22 ; i++) buffer[i] = 0;
	
	if (input == 0) { *buffer = '0'; return buffer; }

	int pos = 0;
	uint64_t tmp = input;

	while (tmp > 0) { tmp /= 8 ; pos++; }

	while (input > 0) {
		pos--;
		buffer[pos] = '0' + (input % 8);
		input /= 8;
	}

	return buffer;
}

//When called, make sure that ino->size == 0 for IDIR!
void initrd_driver::remove(char* path) {
	//printfk("Removing file at %s\n", path);
	for (auto header : *(this->headers)) {
		if (strcmp(header->name, path)) {
			this->headers->del_by_value(header);
			// We didn't allocate the header in the original tarball, so we just leave it allocated. Sucks but eh
			if (header->driver_allocated)
				delete header;
			break;
		}
	}
}
//When called, make sure that ino->size == 0 for IDIR!
void initrd_driver::create(char* path, inode_t* ino) {
	uint64_t size_bytes = 512 + ino->size;
	// Round up to the nearest 512 byte boundary b/c tar
	size_bytes = (size_bytes + 511) & ~ 511;
	uint8_t* buffer = new uint8_t[size_bytes];
	initrd_header_t header = {
		.mode = ino->mode,
		.inode_num = ino->inode_num,
		.driver_allocated = true
	};

	char* uid  = this->to_octal(ino->uid);
	char* gid  = this->to_octal(ino->gid);
	char* size  = this->to_octal(ino->size);
	char* mtime = this->to_octal(ino->mtime);

	memcpy(header.name, path, strlen(path));
	memcpy(header.uid, uid, strlen(uid));
	memcpy(header.gid, gid, strlen(gid));
	memcpy(header.size, size, strlen(size));
	memcpy(header.mtime, mtime, strlen(mtime));

	delete uid;
	delete gid;
	delete size;
	delete mtime;

	memcpy(buffer, &header, sizeof(initrd_header_t));
	this->headers->push_back((initrd_header_t*)buffer);
}

size_t initrd_driver::write(uint8_t* data, inode_t* ino, uint64_t offset, size_t bytes) {
	initrd_header_t* header = this->get_header_by_inode(ino);

	if (header == nullptr) return -1;
	char* name = header->name;

	if (bytes > (ino->size - offset)) {
		// Get a temp copy of the old file
		uint8_t* tmp = new char[ino->size];
		this->read(ino, tmp, ino->size);

		//Remove the old file from the tracked headers
		this->remove(header->name);

		// Set the size to the new expected size and create a new file
		ino->size = bytes + offset;
		this->create(name, ino);

		//Get the new header entry created by this->create()
		header = this->get_header_by_inode(ino);

		// Copy the old file data to the new location. Does this make it CoW?
		uint8_t* old_data_location = ((uint8_t*)header + 512);
		memcpy(old_data_location, tmp, ino->size);
		delete tmp;
	}

	uint8_t* new_data_location = ((uint8_t*)header + 512 + offset);
	memcpy(new_data_location, data, bytes);
	return bytes;
}

//TODO: add a read that includes an offset, or refactor this to include an offset
size_t initrd_driver::read(inode_t* ino, void* buffer, size_t count) {
	// Get the fully qualified path from VFS
	// Since the inode number should be the index in the headers vector + 1
	
	//initrd_header_t* header = this->headers->at(ino->inode_num - 1);
	initrd_header_t* header = this->get_header_by_inode(ino);
	if (header == nullptr) return 0;

	//We're assuming count is in bytes here....
	if (count > this->parse_size(header->size)) count = this->parse_size(header->size);


	uint8_t* data_location = ((uint8_t*)header + 512);
	memcpy(buffer, data_location, count);
	return count;
}

initrd_header_t* initrd_driver::get_header_by_inode(inode_t* ino) {
	for (auto header : *(this->headers)) {
		if (ino->inode_num == header->inode_num) return header;
	}
	return nullptr;
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
	initrd_header_t* cur = (initrd_header_t*)cur_addr;
	while (cur_idx != idx) {
		uint64_t size = this->parse_size(cur->size);
		cur_addr += (( size / 512) + 1) * 512;
		if (size % 512) cur_addr += 512;
		cur = (initrd_header_t*)cur_addr;
		cur_idx++;
	}

	if (cur->name[0] == '\0') return nullptr;
	return cur;

}

void initrd_driver::dump_file_headers() {
	logfk(KERNEL, "Dumping headers\n");
	for (auto header : *(this->headers)) {
		if (header == nullptr) { printfk("FOUND NULL PTR IN HEADERS"); continue; }
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
	fs_ident_t* fs_ident = new fs_ident_t;
	fs_ident->driver = this;

	inode_t* root_inode = new inode_t(fs_ident,0,777,0,0,0,sizeof(inode_t),1,1,1,1,4096, IDIR);
	vnode_t* root_vnode = new vnode_t("/", root_inode);
	uint64_t inode_num = 1;

	fs_ident->mountpoint = root_vnode;
	fs_ident->path = "/";

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
				fs_ident,
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

		//Since header->inode_num isn't part of the tar spec, we need to overwrite it now
		//this value matches what is in the VFS inodes
		header->inode_num = inode_num;
		header->driver_allocated = false;

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
