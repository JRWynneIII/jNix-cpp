#pragma once
#include<cstdint>

typedef enum inode_type {
	IFILE,
	IDIR,
	IDEV,
	IFIFO,
	ISOCKET
} inode_type_t;

class inode_t {
public:
	//Identifier on the filesystem (device inode)
	//TODO: Maybe this should be a pointer? idk how tho
	uint64_t fs_ident; 
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

	inode_t();
	inode_t(uint64_t f, uint64_t i, uint16_t m, uint64_t c, uint64_t mt, uint64_t a, uint64_t s, uint64_t u, uint64_t g, uint64_t l, uint64_t b, uint64_t bs, inode_type_t t) : fs_ident(f), inode_num(i), mode(m), ctime(c), mtime(mt), atime(a), size(s), uid(u), gid(g), nlinks(l), blocks(b), block_size(bs), type(t) {}
};
