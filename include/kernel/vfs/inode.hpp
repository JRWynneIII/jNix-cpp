#pragma once
#include<cstdint>
#include<kernel/devices/device_api.hpp>
//#include<kernel/vfs/vnode.hpp>
//#include<kernel/drivers/fs_driver.hpp>
//#include<kernel/vfs/fs_ident.hpp>

class vnode_t;
class fs_driver_t;

typedef struct fs_ident {
	fs_driver_t* driver;
	vnode_t* mountpoint;
	char* path;
} fs_ident_t;


typedef enum inode_type {
	IFILE,
	IDIR,
	IDEV,
	IFIFO,
	ISOCKET
} inode_type_t;

class inode_t {
public:
	//TODO: Maybe this should be a pointer? idk how tho
	fs_ident_t* fs_ident; 
	uint64_t inode_num;
	uint16_t mode;
	uint64_t ctime;
	uint64_t mtime;
	uint64_t atime;
	uint64_t size;
	uint64_t uid;
	uint64_t gid;
	uint64_t nlinks;
	uint64_t blocks;
	uint64_t block_size;
	inode_type_t type;

	inode_t() {
		this->fs_ident = 0; 
		this->inode_num = 0;
		this->mode = 0;
		this->ctime = 0;
		this->mtime = 0;
		this->atime = 0;
		this->size = 0;
		this->uid = 0;
		this->gid = 0;
		this->nlinks = 0;
		this->blocks = 0;
		this->block_size = 0;
		this->type = IFILE;
	}


	inode_t(fs_ident_t* f, uint64_t i, uint16_t m, uint64_t c, uint64_t mt, uint64_t a, uint64_t s, uint64_t u, uint64_t g, uint64_t l, uint64_t b, uint64_t bs, inode_type_t t) : fs_ident(f), inode_num(i), mode(m), ctime(c), mtime(mt), atime(a), size(s), uid(u), gid(g), nlinks(l), blocks(b), block_size(bs), type(t) {}
};

