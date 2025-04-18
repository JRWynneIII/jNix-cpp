#include<cstdint>
#include<cstddef>
#include<kernel.h>
#include<kernel/memory.h>
#include<kernel/memory/framealloc.hpp>

namespace Memory {
	namespace PMM {
		//We're putting the frame bitmap at the end of the kernel aligned with the next page
		framealloc_t& allocator() {
			static framealloc_t allocator = framealloc_t(4096, (uintptr_t)(&endkernel));
			return allocator;
		}

		uintptr_t alloc_frame() {
			return allocator().alloc_frame();
		}
		uintptr_t alloc_contiguous_frames(uint64_t num) {
			return allocator().alloc_contiguous_frames(num);
		}

		void free_frame(uintptr_t frame) {
			allocator().free_frame(frame);
		}
		void free_contiguous_frames(uintptr_t frames, uint64_t num) {
			allocator().free_contiguous_frames(frames, num);
		}

		void dump_pmm_info() {
			logfk(KERNEL, "PMM: Using %d frames starting at %x\n", allocator().get_num_bitmap_used_frames(), allocator().get_bitmap_address());
		}
		uint64_t num_frames_used() {
			return allocator().get_num_bitmap_used_frames();
		}

		uintptr_t bitmap_location() {
			return allocator().get_bitmap_address();
		}
	}
}
