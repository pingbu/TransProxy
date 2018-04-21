#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#pragma once

#define TRACE()      Utils::Log::d("TRACE at %s(), source %s, line %d", __FUNCTION__, __FILE__, __LINE__)
#define TRACE_THIS() Utils::Log::d("TRACE at %p->%s(), source %s, line %d", this, __FUNCTION__, __FILE__, __LINE__)

namespace Utils {
namespace Log {

extern void setLevel(int level);
extern void __log(const char* tag, int level, const char* fmt, va_list ap);

#ifdef LOG_TAG
#define __log_with_tag(level, fmt, ap)  __log(LOG_TAG, level, fmt, ap);
#else
#define __log_with_tag(level, fmt, ap)
#endif

static inline void v(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	__log_with_tag(4, fmt, ap);
	va_end(ap);
}

static inline void d(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	__log_with_tag(3, fmt, ap);
	va_end(ap);
}

static inline void i(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	__log_with_tag(2, fmt, ap);
	va_end(ap);
}

static inline void w(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	__log_with_tag(1, fmt, ap);
	va_end(ap);
}

static inline void e(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	__log_with_tag(0, fmt, ap);
	va_end(ap);
}

extern void __dump(const char* tag, const void* data, size_t bytes);

static inline void dump(const void* data, size_t bytes) {
#ifdef LOG_TAG
	__dump(LOG_TAG, data, bytes);
#endif
}

}
}
