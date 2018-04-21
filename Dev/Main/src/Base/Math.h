#include <netinet/in.h>
#include <stdint.h>
#include <time.h>
#include "String.h"

#pragma once

namespace Utils {

template<typename T> static inline int compare(T a, T b) {
	return a == b ? 0 : a > b ? 1 : -1;
}

template<typename T> static inline T min(T a, T b) {
	return a <= b ? a : b;
}

template<typename T> static inline T max(T a, T b) {
	return a >= b ? a : b;
}

static inline uint32_t sum(const void* data, size_t bytes) {
	const uint8_t* buf = (const uint8_t*) data;
	uint32_t cksum = 0;
	if ((bytes & 1) != 0)
		cksum = (uint16_t) buf[--bytes] << 8;
	for (size_t i = 0; i < bytes; i += 2)
		cksum += (uint16_t) ntohs(*(uint16_t*) (buf + i));
	return cksum;
}

static inline uint16_t checksum(uint32_t cksum) {
	for (;;) {
		uint16_t hw = cksum >> 16;
		if (hw == 0)
			break;
		cksum = (cksum & 0xFFFF) + hw;
	}
	return htons(~cksum);
}

static inline void formatTime(char* buf, time_t t) {
	struct tm* lt = localtime(&t);
	::sprintf(buf, "%u:%02u:%02u", lt->tm_hour, lt->tm_min, lt->tm_sec);
}

static inline Utils::String formatTime(time_t t) {
	char s[1024];
	formatTime(s, t);
	return s;
}

static inline void formatTimeSpan(char* buf, time_t t) {
	struct tm* lt = gmtime(&t);
	::sprintf(buf, "%u:%02u:%02u", lt->tm_hour, lt->tm_min, lt->tm_sec);
}

static inline Utils::String formatTimeSpan(time_t t) {
	char s[1024];
	formatTimeSpan(s, t);
	return s;
}

static inline void formatSize(char* buf, uint64_t bytes) {
	if (bytes < 1024) {
		::sprintf(buf, "%uB", (uint32_t) bytes);
		return;
	}
	bytes /= 1024;
	if (bytes < 1024) {
		::sprintf(buf, "%uK", (uint32_t) bytes);
		return;
	}
	bytes /= 1024;
	if (bytes < 1024) {
		::sprintf(buf, "%uM", (uint32_t) bytes);
		return;
	}
	bytes /= 1024;
	if (bytes < 1024) {
		::sprintf(buf, "%uG", (uint32_t) bytes);
		return;
	}
	bytes /= 1024;
	::sprintf(buf, "%uT", (uint32_t) bytes);
}

static inline Utils::String formatSize(uint64_t bytes) {
	char s[1024];
	formatSize(s, bytes);
	return s;
}

}
