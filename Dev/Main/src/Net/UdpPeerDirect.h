#include "Base/Utils.h"
#include "Base/Looper.h"
#include "UdpPeer.h"

#pragma once

namespace Net {

class UdpPeerDirect: public UdpPeer, Utils::FDListener {
	UdpPeerListener* _listener;
	uint16_t _port;
	int _socket, _selector;
	void onFDToRead() THROWS;
	void onFDToWrite() THROWS {
	}
	void onFDClosed() THROWS {
	}
	void onFDError(Utils::Exception* e) THROWS {
		THROW(e);
	}
public:
	UdpPeerDirect(UdpPeerListener* listener, uint16_t port = 0) THROWS;
	~UdpPeerDirect();
	uint16_t getPort() const {
		return _port;
	}
	void send(Net::IPv4::SockAddr addr, const void* data, size_t bytes) THROWS;
	struct Factory: UdpPeerFactory {
		UdpPeerDirect* udp_bind(UdpPeerListener* listener, uint16_t port = 0)
				THROWS {
			return new UdpPeerDirect(listener, port);
		}
	};
};

}
