#include <stddef.h>
#include <stdint.h>
#include "Base/Debug.h"
#include "Base/Utils.h"
#include "Net/IPv4.h"
#include "Mac.h"

#pragma once

namespace TransProxy {

class IPv4Protocol {
	friend class IPv4;
	IPv4Protocol* _next;
public:
	virtual ~IPv4Protocol() {
	}
	virtual void dispatchPacket(Net::IPv4::IpPacket& packet) THROWS = 0;
};

class IPv4: public MacProtocol {
	Mac* _mac;
	IPv4Protocol* _protocols = NULL;

public:
	IPv4(Mac* mac) :
			_mac(mac) THROWS {
		Utils::Log::i("IPv4 initializing...");
	}
	virtual ~IPv4() {
		Utils::Log::e("~IPv4");
	}

	void addProtocol(IPv4Protocol* protocol) {
		protocol->_next = _protocols;
		_protocols = protocol;
	}
	void sendPacket(Net::IPv4::IpPacket& packet) {
		_mac->sendPacket(packet.ptr(), packet.packetSize());
	}

	// MacProtocol
	void dispatchPacket(void* packet, size_t bytes) THROWS {
		if (Net::IPv4::IpPacket::isValid(packet)) {
			Net::IPv4::IpPacket in(packet, bytes);
			for (IPv4Protocol* protocol = _protocols; protocol; protocol =
					protocol->_next)
				protocol->dispatchPacket(in);
		}
	}
};

}
