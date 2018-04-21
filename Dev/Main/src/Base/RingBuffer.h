#include <stdint.h>

#pragma once

namespace Utils {

class RingBuffer {
	struct TRing {
		size_t buffer_size, read, write;
		uint8_t buffer[1];
	};

	TRing* mRing;
	bool mNeedFree;

	RingBuffer(TRing* ring, bool needFree) :
			mRing(ring), mNeedFree(needFree) {
	}

	void end_read(size_t bytes) {
		mRing->read = (mRing->read + bytes) % (mRing->buffer_size << 1);
	}
	void end_write(size_t bytes) {
		mRing->write = (mRing->write + bytes) % (mRing->buffer_size << 1);
	}

public:
	static size_t calcTotalSize(size_t ring_size) {
		return (size_t) &((TRing*) 0)->buffer[ring_size];
	}

	static RingBuffer* alloc(size_t ring_size) {
		TRing* ring = (TRing*) new uint8_t[calcTotalSize(ring_size)];
		ring->buffer_size = ring_size;
		ring->read = ring->write = 0;
		return new RingBuffer(ring, true);
	}

	static RingBuffer* attach(void* ptr) {
		return new RingBuffer((TRing*) ptr, false);
	}

	virtual ~RingBuffer() {
		if (mNeedFree)
			delete[] (uint8_t*) mRing;
	}

	size_t available() const;
	size_t free() const;

	size_t peek(size_t offset, size_t bytes, void* buffer);
	size_t peek(void* buffer, size_t bytes) {
		return peek(0, bytes, buffer);
	}

	size_t read(void* buffer, size_t bytes);
	size_t write(const void* buffer, size_t bytes);

	void clear_unsafe() {
		mRing->write = mRing->read;
	}
	void clear_read() {
		mRing->read = mRing->write;
	}
};

}
