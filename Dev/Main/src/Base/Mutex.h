#include <pthread.h>

#pragma once

namespace Utils {

class Mutex {
	::pthread_mutex_t _mutex;
public:
	Mutex() {
		::pthread_mutex_init(&_mutex, NULL);
	}
	virtual ~Mutex() {
		::pthread_mutex_destroy(&_mutex);
	}
	void lock() {
		::pthread_mutex_lock(&_mutex);
	}
	void unlock() {
		::pthread_mutex_unlock(&_mutex);
	}
};

#endif
