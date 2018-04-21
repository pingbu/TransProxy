#include <pthread.h>
#include "Debug.h"

#pragma once

namespace Utils {

class Thread;

struct ThreadProc {
	virtual ~ThreadProc() {
	}
	virtual void* threadProc(Thread* thread, int param_i, void* param_p) = 0;
};

class Thread {
	ThreadProc* _threadProc;
	int _threadParamI;
	void* _threadParamP;
	pthread_t _tid;
	static void* _entry(void* thiz);
public:
	Thread(ThreadProc* proc, int param_i = 0, void* param_p = NULL) THROWS;
	virtual ~Thread() {
		if (_tid != 0) {
			TRY{
				join();
			}CATCH(e) {
				e->print();
			}
		}
	}
	void* join() THROWS;
	void detach() THROWS;
};

}
