#include "Base/Utils.h"
#include "Base/Debug.h"
#include "Net/IPv4.h"
#include "ProxySocksTcp.h"

#pragma once

namespace Net {

class ProxySocks: public TcpClientFactory {
	TcpClientFactory* _factory;
	IPv4::SockAddr _addr;

public:
	ProxySocks(TcpClientFactory* factory, IPv4::SockAddr addr) :
			_factory(factory), _addr(addr) {
	}

	ProxySocksTcp* tcp_open(TcpConnectionListener* listener) THROWS {
		return new ProxySocksTcp(_factory->tcp_open(), _addr, listener);
	}
};

}
