#pragma once
#include<cstdint>
#include<cstddef>
#include<kernel/memory.h>

typedef struct bitmap_idx {
	uint64_t byteidx;
	size_t bitidx;
} bitmap_idx_t;

class framealloc_t {
private:
	uintptr_t base_addr;
	uint64_t frame_size;
	uint8_t* bitmap;
	uint64_t num_frames;
	uint64_t bitmap_size;

	void bitmap_mark_used(uintptr_t addr);
	void bitmap_mark_free(uintptr_t addr);
	bool is_frame_used(bitmap_idx_t idx);

	bitmap_idx_t find_free_frame();
	bitmap_idx_t find_contiguous_frames(uint64_t num);

	uintptr_t idx_to_addr(bitmap_idx_t idx);
	bitmap_idx_t addr_to_idx(uintptr_t addr);
public:
	framealloc_t(uint64_t size, uintptr_t bitmap_location);
	~framealloc_t();

	uintptr_t alloc_frame();
	uintptr_t alloc_contiguous_frames(uint64_t num);

	void free_frame(uintptr_t frame);
	void free_contiguous_frames(uintptr_t frames, uint64_t num);
	uintptr_t get_bitmap_address();
	uint64_t get_num_bitmap_used_frames();
};
