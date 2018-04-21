#include <stdint.h>
#include "Base/Debug.h"
#include "Base/Utils.h"

#pragma once

namespace TransProxy {

struct MacProtocol {
	virtual ~MacProtocol() {
	}
	virtual void dispatchPacket(void* packet, size_t bytes) THROWS = 0;
};

class Mac {
	MacProtocol* _protocol[2];
	size_t _protocols;

public:
	Mac() :
			_protocols(0) THROWS {
		Utils::Log::i("MAC initializing...");
	}
	virtual ~Mac() {
		Utils::Log::e("~Mac");
	}

	void addProtocol(MacProtocol* protocol) {
		_protocol[_protocols++] = protocol;
	}
	virtual void sendPacket(void* packet, size_t bytes) THROWS = 0;

	void dispatchPacket(void* packet, size_t bytes) THROWS {
		for (size_t i = 0; i < _protocols; ++i)
			_protocol[i]->dispatchPacket(packet, bytes);
	}
};

}
