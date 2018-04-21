#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include "Malloc.h"
#include "ThreadLocal.h"
#include "String.h"

#pragma once

#define TRY             for (Utils::_JB curJB; curJB._try(__FUNCTION__, __FILE__, __LINE__); )
#define CATCH(e)        for (Utils::Exception* e = Utils::__catch(__FUNCTION__, __FILE__, __LINE__); e; delete e, e = NULL)

#define THROW(e)        Utils::__lastJB.get()->_throw((e), __FUNCTION__, __FILE__, __LINE__)
#define THROW_IF(c, e)  { if (c) THROW(e); }
#define THROWS

#define ASSERT(c)       THROW_IF(!(c),new Utils::Exception("ASSERT(%s) failed!!!",#c))

namespace Utils {

class Exception {
	String _msg;
	const char* _func = NULL;
	const char* _file = NULL;
	int _line;
public:
	Exception() :
			_line(0) {
	}
	Exception(const char* fmt, ...) :
			_line(0) {
		va_list ap;
		va_start(ap, fmt);
		_msg = String::formatV(fmt, ap);
		va_end(ap);
	}
	const char* getMessage() const {
		return _msg;
	}
	const char* getFunction() const {
		return _func;
	}
	const char* getFile() const {
		return _file;
	}
	int getLine() const {
		return _line;
	}
	void setTrace(const char* func, const char* file, int line);
	void print() const;
};

extern ThreadLocal<struct _JB, false> __lastJB;

struct _JB {
	struct _JB* _prev;
	jmp_buf _jb;
	bool _available;
	_JB() {
		_prev = __lastJB.get();
		__lastJB.set(this);
		_available = true;
	}
	~_JB() {
		__lastJB.set(_prev);
	}
	bool _try(const char* func, const char* file, int line);
	void _throw(Exception* e, const char* func, const char* file, int line);
};

extern Exception* __catch(const char* func, const char* file, int line);

}
