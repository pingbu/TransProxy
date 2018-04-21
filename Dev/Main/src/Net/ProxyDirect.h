#include "TcpClient.h"

#pragma once

namespace Net {

class ProxyDirect: public TcpClientFactory {
	TcpClientFactory* _factory;

public:
	ProxyDirect(TcpClientFactory* factory) :
			_factory(factory) {
	}

	TcpConnection* tcp_open(TcpConnectionListener* listener) THROWS {
		return _factory->tcp_open(listener);
	}
};

}
