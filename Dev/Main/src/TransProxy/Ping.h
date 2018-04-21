#include "Base/Debug.h"
#include "IPv4.h"

#pragma once

namespace TransProxy {

class Ping: public IPv4Protocol {
	IPv4* _ipv4;

public:
	Ping(IPv4* ipv4) :
			_ipv4(ipv4) THROWS {
		Utils::Log::i("Protocol Ping initializing...");
	}
	virtual ~Ping() {
		Utils::Log::e("~Ping");
	}

	// IPv4Protocol
	void dispatchPacket(Net::IPv4::IpPacket& packet) THROWS {
		if (packet.protocol() == IPPROTO_ICMP) {
			//Utils::Log::d("ICMP packet");
			uint32_t src = packet.getSrcAddr();
			uint32_t dst = packet.getDestAddr();
			packet.setSrcAddr(dst);
			packet.setDestAddr(src);

			// 仅仅交换地址数据顺序不需要重新计算校验和
			_ipv4->sendPacket(packet);
		}
	}
};

}
