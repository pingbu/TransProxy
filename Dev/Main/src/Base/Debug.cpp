#define LOG_TAG "Debug"

#include <execinfo.h>
#include "Debug.h"
#include "Log.h"

namespace Utils {

ThreadLocal<struct _JB, false> __lastJB;
static ThreadLocal<Exception, false> __ex;

void Exception::setTrace(const char* func, const char* file, int line) {
	if (_file == NULL) {
		_func = func;
		_file = file;
		_line = line;
	}
}

void Exception::print() const {
	Log::w("EXCEPTION: %s", _msg.sz());
	Log::w("    at %s(), source %s, line %d", _func, _file, _line);
}

bool _JB::_try(const char* func, const char* file, int line) {
	if (_available) {
		_available = false;
		Log::v("try at %s(), source %s, line %d", func, file, line);
		if (::setjmp(_jb) == 0)
			return true;
	}
	return false;
}
void _JB::_throw(Exception* e, const char* func, const char* file, int line) {
	Log::v("throw at %s(), source %s, line %d", func, file, line);
	e->setTrace(func, file, line);
	if (__lastJB.get()) {
		__ex.set(e);
		::longjmp(_jb, 1);
	} else {
		e->print();
		Log::e("throw out of try");
		::abort();
	}
}

Exception* __catch(const char* func, const char* file, int line) {
	Exception* e = Utils::__ex.remove();
	if (e)
		Log::v("catch at %s(), source %s, line %d", func, file, line);
	return e;
}

}
