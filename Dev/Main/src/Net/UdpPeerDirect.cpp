#define LOG_TAG "UdpPeerDirect"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "UdpPeerDirect.h"

namespace Net {

UdpPeerDirect::UdpPeerDirect(UdpPeerListener* listener, uint16_t port) :
		_listener(listener), _port(port) THROWS {
	Utils::Log::i("Direct UDP peer initializing...");

	_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	THROW_IF(_socket < 0, new Utils::Exception("Create UDP socket failed."));

	TRY{
		struct sockaddr_in addr = {0};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(port);
		int r = ::bind(_socket, (const struct sockaddr*) &addr, sizeof(addr));
		THROW_IF(r != 0, new Utils::Exception("Bind UDP socket failed."));

		if (port == 0) {
			size_t n = sizeof(addr);
			r = ::getsockname(_socket, (struct sockaddr*) &addr, &n);
			THROW_IF (r != 0,
					new Utils::Exception("Bind failed, errno=%d.", errno));
			_port = ntohs(addr.sin_port);
		}

		_selector = Utils::Looper::myLooper()->attachFD(_socket, this);
		Utils::Log::i("Bound UDP at port %d, selector #%d", _port, _selector);
		Utils::Looper::myLooper()->waitToRead(_selector);
	}CATCH (e){
		::close(_socket);
		THROW(e);
	}
}

UdpPeerDirect::~UdpPeerDirect() {
	Utils::Looper::myLooper()->detachFD(_selector);
	int s = _socket;
	_socket = -1;
	::close(s);
}

void UdpPeerDirect::onFDToRead() THROWS {
	unsigned char buf[65536];
	struct sockaddr_in sa = { 0 };
	socklen_t n = sizeof(sa);
	ssize_t bytes = ::recvfrom(_socket, buf, sizeof(buf), 0,
			(struct sockaddr*) &sa, &n);
	if (bytes < 0) {
		THROW_IF(_socket != -1, new Utils::Exception("UDP receive FAILED"));
		Utils::Looper::myLooper()->waitToRead(_selector);
		return;
	}

	Net::IPv4::SockAddr addr = sa;
	Utils::Log::v("%s:%u ----> %u bytes", Net::IPv4::ntoa(addr.ip), addr.port,
			bytes);
	Utils::Log::dump(buf, bytes);
	_listener->onReceived(addr, buf, bytes);
	Utils::Looper::myLooper()->waitToRead(_selector);
}

void UdpPeerDirect::send(Net::IPv4::SockAddr addr, const void* data,
		size_t bytes) THROWS {
	Utils::Log::v("%s:%u <---- %u bytes", Net::IPv4::ntoa(addr.ip), addr.port,
			bytes);
	Utils::Log::dump(data, bytes);
	struct sockaddr_in sa = { 0 };
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(addr.ip);
	sa.sin_port = htons(addr.port);
	ssize_t r = ::sendto(_socket, data, bytes, 0, (const struct sockaddr*) &sa,
			sizeof(sa));
	if (r < 0)
		Utils::Log::w("UDP send FAILED, errno=%d", errno);
}

}
