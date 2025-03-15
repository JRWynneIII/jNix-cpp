#include<atomic>

class spinlock_t {
private:
	std::atomic_flag lock;
public:
	spinlock_t();
	~spinlock_t();
	void aquire();
	void release();
	bool is_free();
};
