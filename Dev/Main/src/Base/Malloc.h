#include <stddef.h>
#include <stdint.h>
#include <time.h>

#pragma once

struct MallocBlock {
	MallocBlock *prev, *next;
	const char* tag;
	time_t time;
	size_t bytes;
	const char* file;
	int line;
	uint8_t buffer[1];
};

extern MallocBlock* __firstMallocBlock;

void* operator new(size_t bytes, const char* file, int line);
void* operator new[](size_t bytes, const char* file, int line);
void operator delete(void* ptr);
void operator delete[](void* ptr);

#ifndef IN_MALLOC
#define new new(__FILE__, __LINE__)
#endif

void setDefaultMallocTag(const char* tag);
