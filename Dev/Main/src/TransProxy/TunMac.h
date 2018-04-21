#include <stdint.h>
#include "Base/Debug.h"
#include "Base/Utils.h"
#include "Net/Tun.h"
#include "Mac.h"

#pragma once

namespace TransProxy {

class TunMac: public Mac, Net::TunListener {
	Net::Tun _tun;

	// Net::TunListener
	void onTunReceived(void* packet, size_t bytes) THROWS {
		Mac::dispatchPacket(packet, bytes);
	}
	void onTunError(Utils::Exception* e) THROWS {
		THROW(e);
	}

public:
	TunMac(const char* ip, const char* mask) :
			_tun(Net::IPv4::aton(ip), Net::IPv4::aton(mask), this) THROWS {
		Utils::Log::i("TUN MAC initializing...");
	}
	virtual ~TunMac() {
		Utils::Log::e("~TunMac");
	}

	void sendPacket(void* packet, size_t bytes) THROWS {
		_tun.send(packet, bytes);
	}
};

}
