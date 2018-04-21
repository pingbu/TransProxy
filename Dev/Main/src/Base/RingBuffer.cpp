#include <memory.h>
#include "Math.h"
#include "Log.h"
#include "RingBuffer.h"

namespace Utils {

size_t RingBuffer::available() const {
	size_t r = mRing->read;
	size_t w = mRing->write;

	size_t r_page = r / mRing->buffer_size;
	size_t r_offset = r % mRing->buffer_size;
	size_t w_page = w / mRing->buffer_size;
	size_t w_offset = w % mRing->buffer_size;

	size_t l = w_offset - r_offset;
	if (w_page != r_page)
		l += mRing->buffer_size;
	return l;
}

size_t RingBuffer::free() const {
	size_t r = mRing->read;
	size_t w = mRing->write;

	size_t r_page = r / mRing->buffer_size;
	size_t r_offset = r % mRing->buffer_size;
	size_t w_page = w / mRing->buffer_size;
	size_t w_offset = w % mRing->buffer_size;

	size_t l = r_offset - w_offset;
	if (w_page == r_page)
		l += mRing->buffer_size;
	return l;
}

size_t RingBuffer::peek(size_t offset, size_t bytes, void* buffer) {
	size_t r = (mRing->read + offset) % (mRing->buffer_size << 1);
	size_t w = mRing->write;

	size_t r_page = r / mRing->buffer_size;
	size_t r_offset = r % mRing->buffer_size;
	size_t w_page = w / mRing->buffer_size;
	size_t w_offset = w % mRing->buffer_size;

	if (w_page == r_page) {
		size_t l = w_offset - r_offset;
		if (bytes > l)
			bytes = l;
		::memcpy(buffer, mRing->buffer + r_offset, bytes);
	} else {
		size_t l = mRing->buffer_size - r_offset;
		if (bytes <= l) {
			::memcpy(buffer, mRing->buffer + r_offset, bytes);
		} else {
			::memcpy(buffer, mRing->buffer + r_offset, l);
			size_t l2 = Utils::min(bytes - l, w_offset);
			::memcpy((uint8_t*) buffer + l, mRing->buffer, l2);
			bytes = l + l2;
		}
	}
	return bytes;
}

size_t RingBuffer::read(void* buffer, size_t bytes) {
	size_t l = peek(buffer, bytes);
	end_read(l);
	return l;
}

size_t RingBuffer::write(const void* buffer, size_t bytes) {
	size_t r = mRing->read;
	size_t w = mRing->write;

	size_t r_page = r / mRing->buffer_size;
	size_t r_offset = r % mRing->buffer_size;
	size_t w_page = w / mRing->buffer_size;
	size_t w_offset = w % mRing->buffer_size;

	if (w_page != r_page) {
		size_t l = r_offset - w_offset;
		if (bytes > l)
			bytes = l;
		::memcpy(mRing->buffer + w_offset, buffer, bytes);
	} else {
		size_t l = mRing->buffer_size - w_offset;
		if (bytes <= l) {
			::memcpy(mRing->buffer + w_offset, buffer, bytes);
		} else {
			::memcpy(mRing->buffer + w_offset, buffer, l);
			size_t l2 = Utils::min(bytes - l, r_offset);
			::memcpy(mRing->buffer, (uint8_t*) buffer + l, l2);
			bytes = l + l2;
		}
	}
	end_write(bytes);
	return bytes;
}

}
