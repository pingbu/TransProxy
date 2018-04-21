#include <pthread.h>

namespace Utils {

template<class T, bool AutoDelete = true>
class ThreadLocal {
	::pthread_key_t _key;
	static void __destroy(void* value) {
		if (AutoDelete)
			delete (T*) value;
	}
public:
	ThreadLocal() {
		::pthread_key_create(&_key, __destroy);
	}
	virtual ~ThreadLocal() {
		::pthread_key_delete(_key);
	}
	T* get() const {
		return (T*) ::pthread_getspecific(_key);
	}
	T* remove() {
		T* value = (T*) ::pthread_getspecific(_key);
		if (value != NULL)
			::pthread_setspecific(_key, NULL);
		return value;
	}
	void set(T* value) {
		T* prev = (T*) ::pthread_getspecific(_key);
		if (AutoDelete && prev != NULL)
			delete prev;
		::pthread_setspecific(_key, value);
	}
};

}
