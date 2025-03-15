//#include<atomic>
//#include<spinlock.hpp>
//
//spinlock_t::spinlock_t() {
//	//TODO what
//	this->lock = std::atomic_flag();
//}
//
//void spinlock_t::aquire() {
//	//Disable interrupts on cpu that this spinlock is on
//	while(atomic_flag_test_and_set(this->lock)) {
//		// TODO: does this actually work?
//		__builtin_ia32_pause();
//	}
//}
//
//void spinlock_t::release() {
//	atomic_flag_clear(this->lock);
//}
//
//void spinlock_t::is_free() {
//	//TODO: find actual function to test lock
//	if (atomic_flag_test(this->lock)) {
//		//If is free
//		return true;
//	}
//	return false;
//}
