#include <stdint.h>
#include "Base/Debug.h"

#pragma once

namespace Net {

enum {
	PROTO_NULL = -1, PROTO_TCP = 0, PROTO_UDP, PROTO_COUNT
};

class Packet {
	uint8_t* _ptr;
	size_t _size;

public:
	Packet(void* ptr, size_t size) :
			_ptr((uint8_t*) ptr), _size(size) {
	}
	virtual ~Packet() {
	}
	size_t size() const {
		return _size;
	}
	uint8_t* ptr() {
		return _ptr;
	}
	const uint8_t* ptr() const {
		return _ptr;
	}
	uint8_t& operator[](size_t index) {
		ASSERT(index < _size);
		return _ptr[index];
	}
	uint8_t operator[](size_t index) const {
		ASSERT(index < _size);
		return _ptr[index];
	}
	uint16_t read16(size_t offset) const {
		ASSERT(offset + sizeof(uint16_t) <= _size);
		return ntohs(*(uint16_t*) (_ptr + offset));
	}
	uint32_t read32(size_t offset) const {
		ASSERT(offset + sizeof(uint32_t) <= _size);
		return ntohl(*(uint32_t*) (_ptr + offset));
	}
	void read(size_t offset, void* data, size_t bytes) const {
		ASSERT(offset + bytes <= _size);
		::memcpy(data, _ptr + offset, bytes);
	}
	void write16(size_t offset, uint16_t v) {
		ASSERT(offset + sizeof(uint16_t) <= _size);
		*(uint16_t*) (_ptr + offset) = htons(v);
	}
	void write32(size_t offset, uint32_t v) {
		ASSERT(offset + sizeof(uint32_t) <= _size);
		*(uint32_t*) (_ptr + offset) = htonl(v);
	}
	void write(size_t offset, const void* data, size_t bytes) {
		ASSERT(offset + bytes <= _size);
		::memcpy(_ptr + offset, data, bytes);
	}
};

template<class Packet>
class PacketBuffer: public Packet {
	uint8_t* _buf;

public:
	PacketBuffer(size_t n) :
			Packet(_buf = new uint8_t[n], n) {
	}
	~PacketBuffer() {
		delete[] _buf;
	}
};

template<class Packet, size_t N>
class PacketFixedBuffer: public Packet {
	uint8_t _buf[N];

public:
	PacketFixedBuffer() :
			Packet(_buf, N) {
	}
};

}
