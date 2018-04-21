#include <stdio.h>
#include <string.h>
#include <time.h>
#include "Log.h"

namespace Utils {
namespace Log {

static int LOG_LEVEL = 3;

void setLevel(int level) {
	LOG_LEVEL = level;
}

void __log(const char* tag, int level, const char* fmt, va_list ap) {
	if (level < LOG_LEVEL) {
		char s[1024];
		::vsprintf(s, fmt, ap);
		time_t t = ::time(NULL);
		tm* lt = ::localtime(&t);
		::fprintf(stderr, "\033[0;3%c" "m%02u:%02u:%02u %c/%s: %s\n",
				"13724"[level], lt->tm_hour, lt->tm_min, lt->tm_sec,
				"EWIDV"[level], tag, s);
		::fflush(stderr);
	}
}

void __dump(const char* tag, const void* data, size_t bytes) {
	if (LOG_LEVEL > 5) {
		time_t t = ::time(NULL);
		tm* lt = ::localtime(&t);
		const unsigned char* buf = (const unsigned char*) data;
		for (size_t i = 0; i < bytes; i += 16) {
			char s[200];
			::sprintf(s, "\033[0;37;2m%02u:%02u:%02u M/%s:   ", lt->tm_hour,
					lt->tm_min, lt->tm_sec, tag);
			char* p = s + ::strlen(s);
			::sprintf(p, "%04X ", i);
			p += ::strlen(p);
			for (size_t j = 0; j < 8; ++j) {
				if (i + j < bytes)
					::sprintf(p, " %02X", buf[i + j]);
				else
					::sprintf(p, "   ");
				p += ::strlen(p);
			}
			if (i + 8 < bytes)
				*p++ = '-';
			else
				*p++ = ' ';
			for (size_t j = 8; j < 16; ++j) {
				if (i + j < bytes)
					::sprintf(p, "%02X ", buf[i + j]);
				else
					::sprintf(p, "   ");
				p += ::strlen(p);
			}
			*p++ = ' ';
			for (size_t j = 0; j < 16 && i + j < bytes; ++j)
				if (buf[i + j] >= 0x20 && buf[i + j] < 0x80)
					*p++ = buf[i + j];
				else
					*p++ = '.';
			*p++ = '\n';
			*p++ = '\0';
			::fputs(s, ::stderr);
		}
		::fflush(::stderr);
	}
}

}
}
