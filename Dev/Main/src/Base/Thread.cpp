#define LOG_TAG "Thread"

#include <stdlib.h>
#include "Log.h"
#include "Thread.h"

namespace Utils {

void* Thread::_entry(void* param) {
	Thread* thiz = (Thread*) param;
	TRY{
	return thiz->_threadProc->threadProc(thiz, thiz->_threadParamI,
			thiz->_threadParamP);
}
	CATCH(e){
	Log::e("FATAL: Thread %u exit with exception!!!", ::pthread_self());
	e->print();
	::abort();
}
	return NULL;
}

Thread::Thread(ThreadProc* proc, int param_i, void* param_p) THROWS {
	_threadProc = proc;
	_threadParamI = param_i;
	_threadParamP = param_p;
	_tid = 0;
	int r = ::pthread_create(&_tid, NULL, _entry, this);
	THROW_IF(r != 0, new Exception("pthread_create FAILED!"));
}

void Thread::detach() THROWS {
	THROW_IF(_tid == 0, new Exception("pthread_detach on destroyed thread!"));
	int r = ::pthread_detach(_tid);
	THROW_IF(r != 0, new Exception("pthread_detach(%u) FAILED!", _tid));
	_tid = 0;
}

void* Thread::join() THROWS {
	THROW_IF(_tid == 0, new Exception("pthread_join on destroyed thread!"));
	void* exitCode = NULL;
	int r = ::pthread_join(_tid, &exitCode);
	THROW_IF(r != 0, new Exception("pthread_join(%u) FAILED!", _tid));
	_tid = 0;
	return exitCode;
}

}
