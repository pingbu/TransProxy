#include <fcntl.h>
#include "Base/Utils.h"
#include "Base/Looper.h"

#pragma once

namespace Net {

struct TunListener {
	virtual ~TunListener() {
	}
	virtual void onTunReceived(void* packet, size_t bytes) THROWS = 0;
	virtual void onTunError(Utils::Exception* e) THROWS = 0;
};

class Tun: Utils::FDListener {
	TunListener* _listener;
	int _fd, _selector;
	Utils::String _name;
	void onFDToRead() THROWS;
	void onFDToWrite() {
	}
	void onFDClosed() {
	}
	void onFDError(Utils::Exception* e) THROWS {
		THROW(e);
	}
public:
	Tun(uint32_t ip, uint32_t mask, TunListener* listener) THROWS;
	~Tun();
	void send(void* packet, size_t bytes) THROWS;
	const char* getName() const {
		return _name;
	}
};

}
