#define LOG_TAG "TUN"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/if_tun.h>
#include "Net/IPv4.h"
#include "Tun.h"

namespace Net {

Tun::Tun(uint32_t ip, uint32_t mask, TunListener* listener) :
		_listener(listener) THROWS {
	Utils::Log::i("TUN initializing...");

	_fd = ::open("/dev/net/tun", O_RDWR);
	THROW_IF(_fd < 0, new Utils::Exception("Open TUN failed."));
	Utils::Log::i("TUN device open.");

	TRY{
		struct ifreq ifr = {0};
		ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
		int r = ::ioctl(_fd, TUNSETIFF, &ifr);
		THROW_IF(r < 0,
				new Utils::Exception("Create TUN device failed, errno=%d", errno));

		_name = ifr.ifr_name;
		Utils::Log::i("TUN '%s' created.", ifr.ifr_name);

		int s = ::socket(PF_INET, SOCK_STREAM, 0);
		THROW_IF(s < 0, new Utils::Exception("Error create socket"));

		TRY {
			::bzero(&ifr, sizeof(ifr));
			::strcpy(ifr.ifr_name, _name);
			int r = ::ioctl(s, SIOCGIFFLAGS, &ifr);
			THROW_IF(r < 0, new Utils::Exception("Error get flags, errno=%d", errno));
			ifr.ifr_ifru.ifru_flags |= IFF_UP;
			r = ::ioctl(s, SIOCSIFFLAGS, &ifr);
			THROW_IF(r < 0, new Utils::Exception("Error set flags, errno=%d", errno));
			Utils::Log::i("TUN '%s' enabled.", _name.sz());

			::bzero(&ifr, sizeof(ifr));
			::strcpy(ifr.ifr_name, _name);
			ifr.ifr_addr.sa_family = AF_INET;
			((sockaddr_in*) &ifr.ifr_addr)->sin_addr.s_addr = htonl(ip);
			r = ::ioctl(s, SIOCSIFADDR, &ifr);
			THROW_IF(r < 0,
					new Utils::Exception("Error set address, errno=%d", errno));
			Utils::Log::i("TUN '%s' address set to %s", _name.sz(),
					Net::IPv4::ntoa(ip));

			::bzero(&ifr, sizeof(ifr));
			::strcpy(ifr.ifr_name, _name);
			ifr.ifr_netmask.sa_family = AF_INET;
			((sockaddr_in*) &ifr.ifr_netmask)->sin_addr.s_addr = htonl(mask);
			r = ::ioctl(s, SIOCSIFNETMASK, &ifr);
			THROW_IF(r < 0,
					new Utils::Exception("Error set network mask, errno=%d", errno));
			Utils::Log::i("TUN '%s' network mask set to %s", _name.sz(),
					Net::IPv4::ntoa(mask));

			::close(s);
		}CATCH(e) {
			::close(s);
			THROW(e);
		}

		_selector = Utils::Looper::myLooper()->attachFD(_fd, this);
		Utils::Log::i("TUN '%s' running, selector #%d...", _name.sz(), _selector);
		Utils::Looper::myLooper()->waitToRead(_selector);
	}CATCH(e){
		::close(_fd);
		THROW(e);
	}
}

Tun::~Tun() {
	Utils::Looper::myLooper()->detachFD(_selector);
	::close(_fd);
	_fd = -1;
}

void Tun::send(void* packet, size_t bytes) THROWS {
	const uint8_t* buf = (const uint8_t*) packet;
	if (buf[9] == IPPROTO_TCP || buf[9] == IPPROTO_UDP) {
		size_t l = (buf[0] & 0x0F) * 4;
		Utils::Log::v("'%s' %u.%u.%u.%u:%u -- P%u[%u] --> %u.%u.%u.%u:%u",
				(const char*) _name, buf[12], buf[13], buf[14], buf[15],
				ntohs(*(uint16_t*) (buf + l)), buf[9], bytes, buf[16], buf[17],
				buf[18], buf[19], ntohs(*(uint16_t*) (buf + l + 2)));
	} else {
		Utils::Log::v("'%s' %u.%u.%u.%u -- P%u[%u] --> %u.%u.%u.%u",
				(const char*) _name, buf[12], buf[13], buf[14], buf[15], buf[9],
				bytes, buf[16], buf[17], buf[18], buf[19]);
	}
	Utils::Log::dump(packet, bytes);

	int r = ::write(_fd, packet, bytes);
	THROW_IF(r != (int )bytes,
			new Utils::Exception("Error when send %u bytes", bytes));
}

void Tun::onFDToRead() {
	uint8_t buf[1500];
	int r = ::read(_fd, buf, sizeof(buf));
	if (r < 0)
		return;

	if (buf[9] == IPPROTO_TCP || buf[9] == IPPROTO_UDP) {
		size_t l = (buf[0] & 0x0F) * 4;
		Utils::Log::v("'%s' %u.%u.%u.%u:%u <-- P%u[%u] -- %u.%u.%u.%u:%u",
				(const char*) _name, buf[16], buf[17], buf[18], buf[19],
				ntohs(*(uint16_t*) (buf + l + 2)), buf[9], r, buf[12], buf[13],
				buf[14], buf[15], ntohs(*(uint16_t*) (buf + l)));
	} else {
		Utils::Log::v("'%s' %u.%u.%u.%u <-- P%u[%u] -- %u.%u.%u.%u",
				(const char*) _name, buf[16], buf[17], buf[18], buf[19], buf[9],
				r, buf[12], buf[13], buf[14], buf[15]);
	}
	Utils::Log::dump(buf, r);
	_listener->onTunReceived(buf, r);
	Utils::Looper::myLooper()->waitToRead(_selector);
}

}
