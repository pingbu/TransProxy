#define IN_MALLOC

#include <malloc.h>
#include "Malloc.h"

static const char* __defaultMallocTag = NULL;

MallocBlock* __firstMallocBlock = NULL;

void setDefaultMallocTag(const char* tag) {
	__defaultMallocTag = tag;
}

static void* __new(size_t bytes, const char* file, int line) {
	MallocBlock* block = (MallocBlock*) ::malloc(
			(size_t) ((MallocBlock*) 0)->buffer + bytes);
	block->tag = __defaultMallocTag;
	block->time = ::time(NULL);
	block->bytes = bytes;
	block->file = file;
	block->line = line;
	block->prev = NULL;
	block->next = __firstMallocBlock;
	if (__firstMallocBlock)
		__firstMallocBlock->prev = block;
	__firstMallocBlock = block;
	return block->buffer;
}

static void __delete(void* ptr) {
	MallocBlock* block = (MallocBlock*) ((uint8_t*) ptr
			- (size_t) ((MallocBlock*) 0)->buffer);
	MallocBlock* prev = block->prev;
	MallocBlock* next = block->next;
	if (prev)
		prev->next = next;
	else
		__firstMallocBlock = next;
	if (next)
		next->prev = prev;
	::free(block);
}

void* operator new(size_t bytes, const char* file, int line) {
	return __new(bytes, file, line);
}

void* operator new[](size_t bytes, const char* file, int line) {
	return __new(bytes, file, line);
}

void operator delete(void* ptr) {
	__delete(ptr);
}

void operator delete[](void* ptr) {
	__delete(ptr);
}
