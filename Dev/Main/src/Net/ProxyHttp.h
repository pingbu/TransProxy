#include "Base/Utils.h"
#include "Base/Debug.h"
#include "Net/IPv4.h"
#include "ProxyHttpTcp.h"

#pragma once

namespace Net {

class ProxyHttp: public TcpClientFactory {
	TcpClientFactory* _factory;
	IPv4::SockAddr _addr;

public:
	ProxyHttp(TcpClientFactory* factory, IPv4::SockAddr addr) :
			_factory(factory), _addr(addr) {
	}

	ProxyHttpTcp* tcp_open(TcpConnectionListener* listener) THROWS {
		return new ProxyHttpTcp(_factory->tcp_open(), _addr, listener);
	}
};

}
