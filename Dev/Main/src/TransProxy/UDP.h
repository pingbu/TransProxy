#include "Base/Debug.h"
#include "Base/Map.h"
#include "Net/UdpPeer.h"
#include "IPv4.h"

#pragma once

namespace TransProxy {

class UDP: public IPv4Protocol {
	struct _Socket: Net::UdpPeer, Utils::MapItem<Net::IPv4::SockAddr> {
		UDP* _this;
		Net::IPv4::SockAddr _addr;
		Net::UdpPeerListener* _listener;
		uint16_t _id;
		_Socket(UDP* thiz, Net::IPv4::SockAddr addr,
				Net::UdpPeerListener* listener) :
				_this(thiz), _addr(addr), _listener(listener), _id(1) {
			_this->_sockets.add(this);
		}
		~_Socket() {
			_this->_sockets.remove(this);
		}

		// MapItem
		Net::IPv4::SockAddr getKey() const {
			return _addr;
		}
		Utils::String getKeyString() const {
			return _addr.toString();
		}

		// Net::UdpPeer
		uint16_t getPort() const {
			return _addr.port;
		}
		void send(Net::IPv4::SockAddr addr, const void* data, size_t bytes)
				THROWS;
	};

	IPv4* _ipv4;
	Utils::Map<Net::IPv4::SockAddr, _Socket> _sockets;

public:
	UDP(IPv4* ipv4) :
			_ipv4(ipv4) THROWS {
		Utils::Log::i("Protocol UDP initializing...");
	}
	virtual ~UDP() {
		Utils::Log::e("~UDP");
	}

	// IPv4Protocol
	void dispatchPacket(Net::IPv4::IpPacket& packet) THROWS;

	Net::UdpPeer* bind(Net::IPv4::SockAddr addr,
			Net::UdpPeerListener* listener) {
		return new _Socket(this, addr, listener);
	}
};

}
