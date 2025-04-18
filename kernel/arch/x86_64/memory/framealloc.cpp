#include<kernel.h>
#include<cstddef>
#include<cstdint>
#include<kernel/memory.h>
#include<kernel/panic.h>
#include<kernel/memory/framealloc.hpp>


framealloc_t::framealloc_t(uint64_t size, uintptr_t bitmap_location) : frame_size(size) {
	logfk(KERNEL, "PMM: Initializing PMM\n");
	// Get total memory size in bytes
	uint64_t total_mem_bytes = Memory::total_mem_bytes;
	this->num_frames = total_mem_bytes / this->frame_size;
	logfk(KERNEL, "PMM: Frames avail: %d\n", this->num_frames);
	this->bitmap_size = this->num_frames / 8;
	//Round up just in case we have a non divisible number of frames
	this->bitmap_size = (this->num_frames % 8 != 0 ? this->bitmap_size + 1 : this->bitmap_size);
	logfk(KERNEL, "PMM: bitmap_size: %d\n", this->bitmap_size);
	
	//Allocate bitmap
	// Search for usable region to store our bitmap
	logfk(KERNEL, "PMM: Searching for usable frames to store bitmap\n");
	bool found_region = false;
	for (auto i : Memory::usable_memory_regions) {
		if ((i.base + i.length) > 0x100000) {
			if (i.length >= this->bitmap_size) {
				found_region = true;
				this->bitmap = (uint8_t*)(i.base | Memory::hhdm_offset);
			}
		}
	}

	if (!found_region)
		kpanic("Could not find suitable region for PMM bitmap!");

	logfk(KERNEL, "PMM: bitmap location: %x\n", this->bitmap);

	//Mark all frames as used, then mark all 'usable space' as free
	for (int i = 0 ; i < this->bitmap_size; i++) 
		this->bitmap[i] = 0xFF;

	for (auto region : Memory::usable_memory_regions) {
		if (region.length > 0) {
			logfk(KERNEL, "PMM: Adding region starting at %x with length %x\n", region.base, region.length);
			uintptr_t cur = region.base;
			while (cur < (region.base + region.length)) {
				this->bitmap_mark_free(cur);
				cur += this->frame_size;
			}
		}
	}

	//Mark the bitmap frame(s) as used
	uintptr_t first_bitmap_frame = ((uintptr_t)this->bitmap ^ Memory::hhdm_offset) & ~0xFFF; 
	uintptr_t cur_bitmap_frame = first_bitmap_frame; 
	logfk(KERNEL, "PMM: Marking %u frames as used (cur_bitmap_frame: %x)\n", this->bitmap_size/4096, cur_bitmap_frame);
	while(cur_bitmap_frame <= (first_bitmap_frame + this->bitmap_size)) {
		this->bitmap_mark_used(cur_bitmap_frame);
		//logfk(KERNEL, "PMM: bitmap frame (%x) marked as used\n", (cur_bitmap_frame & ~0xFFF));
		cur_bitmap_frame += this->frame_size;
	}

	logfk(KERNEL, "PMM: Init complete\n");
}

framealloc_t::~framealloc_t() {
}

uintptr_t framealloc_t::get_bitmap_address() {
	return (uintptr_t)this->bitmap;
}

uint64_t framealloc_t::get_num_bitmap_used_frames() {
	return (this->bitmap_size % this->frame_size == 0 ? this->bitmap_size / this->frame_size : (this->bitmap_size/this->frame_size) + 1);
}

bool framealloc_t::is_frame_used(bitmap_idx_t idx) {
	return (this->bitmap[idx.byteidx] & (1 << idx.bitidx));
}

bitmap_idx_t framealloc_t::find_free_frame() {
	for (int byteidx = 0 ; byteidx < this->bitmap_size ; byteidx++) {
		for (int bitidx = 0 ; bitidx < 8 ; bitidx++) {
			bitmap_idx_t idx = {byteidx, bitidx};
			if (!(this->is_frame_used(idx))) {
				return idx;
			}
		}
	}
	kpanic("Out of Memory!");
}

bitmap_idx_t framealloc_t::find_contiguous_frames(uint64_t num) {
	bitmap_idx_t idx = {0, 0};
	bool found = false;
	for (int byteidx = 0 ; byteidx < this->bitmap_size ; byteidx++) {
		idx.byteidx = byteidx;
		for (int bitidx = 0 ; bitidx < 8 ; bitidx++) {
			idx.bitidx = bitidx;
			if (!(this->is_frame_used(idx))) {
				found = true;
				bitmap_idx_t next = idx;
				for (int i = 0 ; i < num ; i++) {
					if (next.bitidx + 1 > 8) {
						next.byteidx++;
						next.bitidx = 0;
					} else {
						next.bitidx++;
					}
					if (this->is_frame_used(next)) {
						found = false;
						break;
					}
				}
				if (found) {
					return idx;
				}
			} else {
				found = false;
			}
		}
	}
	//Technically we could do some kind of compaction here, but i don't feel like writing that right now so we just bail
	kpanic("Out of Memory!");
}

void framealloc_t::bitmap_mark_used(uintptr_t addr) {
	//Align to 4k boundary
	addr &= ~0xFFF;
	bitmap_idx_t idx = this->addr_to_idx(addr);

	this->bitmap[idx.byteidx] |= ((uint8_t)1 << idx.bitidx);
}

void framealloc_t::bitmap_mark_free(uintptr_t addr) {
	//Align to 4k boundary
	addr &= ~0xFFF;
	bitmap_idx_t idx = this->addr_to_idx(addr);

	this->bitmap[idx.byteidx] &= ~((uint8_t)1 << idx.bitidx);
}

bitmap_idx_t framealloc_t::addr_to_idx(uintptr_t addr) {
	bitmap_idx_t idx = {0,0};
	idx.bitidx = (addr / this->frame_size) % 8;
	idx.byteidx = (addr / (this->frame_size * 8));
	return idx;
}

uintptr_t framealloc_t::idx_to_addr(bitmap_idx_t idx) {
	return (uintptr_t)(((idx.byteidx * 8) + idx.bitidx) * this->frame_size);
}

uintptr_t framealloc_t::alloc_frame() {
	bitmap_idx_t frame = this->find_free_frame();
	uintptr_t addr = this->idx_to_addr(frame);
	this->bitmap_mark_used(addr);
	return addr;
}

uintptr_t framealloc_t::alloc_contiguous_frames(uint64_t num) {
	bitmap_idx_t frames = this->find_contiguous_frames(num);
	uintptr_t ret = this->idx_to_addr(frames);
	uintptr_t cur = ret;
	for (int i = 0; i < num; i++) {
		this->bitmap_mark_used(cur);
		cur += this->frame_size;
	}
	return ret;
}

void framealloc_t::free_frame(uintptr_t frame) {
	this->bitmap_mark_free(frame);
}

void framealloc_t::free_contiguous_frames(uintptr_t frame, uint64_t num) {
	for(int i = 0; i < num ; i++) {
		this->bitmap_mark_free(frame);
		frame += this->frame_size;
	}
}
