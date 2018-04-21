#include <stdint.h>
#include "Base/Debug.h"

#pragma once

namespace TransProxy {

struct ProxyAuth {
	virtual ~ProxyAuth() {
	}
	virtual void sendAuthRequest() THROWS = 0;
	virtual int onAuthResponse(const void* data, size_t bytes) THROWS = 0;
};

struct ProxyAuthListener {
	virtual ~ProxyAuthListener() {
	}
	virtual void sendAuthRequest(const void* data, size_t bytes,
			size_t bufferSize) THROWS = 0;
	virtual void commitAuthRequest(size_t bytes) THROWS = 0;
};

struct ProxyAuthBuilder {
	virtual ~ProxyAuthBuilder() {
	}
	virtual ProxyAuth* createInstance(const char* hostname, uint16_t port,
			ProxyAuthListener* listener) THROWS = 0;
};

}
