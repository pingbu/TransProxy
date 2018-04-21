#include <stdio.h>
#include <stdint.h>
#include "Base/Utils.h"
#include "Net/IPv4.h"

#pragma once

namespace Net {

struct UdpPeer {
	virtual ~UdpPeer() {
	}
	virtual uint16_t getPort() const = 0;
	virtual void send(Net::IPv4::SockAddr addr, const void* data, size_t bytes)
			THROWS = 0;
};

struct UdpPeerListener {
	virtual ~UdpPeerListener() {
	}
	virtual void onReceived(Net::IPv4::SockAddr addr, void* data, size_t bytes)
			THROWS = 0;
	virtual void onError(Utils::Exception* e) THROWS = 0;
};

struct UdpPeerFactory {
	virtual ~UdpPeerFactory() {
	}
	virtual UdpPeer* udp_bind(UdpPeerListener* listener, uint16_t port = 0)
			THROWS = 0;
};

}
