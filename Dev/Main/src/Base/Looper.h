#include "Debug.h"

#pragma once

namespace Utils {

struct FDListener {
	virtual ~FDListener() {
	}
	virtual void onFDToRead() THROWS = 0;
	virtual void onFDToWrite() THROWS = 0;
	virtual void onFDClosed() THROWS = 0;
	virtual void onFDError(Utils::Exception* e) THROWS = 0;
};

struct Looper {
	static void prepare() THROWS;
	static void loopOnce() THROWS;
	static void loop() THROWS;
	static Looper* myLooper();

	int attachFD(int fd, FDListener* listener) THROWS;
	void detachFD(int index);

	void waitToRead(int index);
	void waitToWrite(int index);
};

}
