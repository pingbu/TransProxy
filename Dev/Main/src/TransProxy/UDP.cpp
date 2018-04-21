#include "Base/Debug.h"
#include "Base/Utils.h"
#include "UDP.h"

namespace TransProxy {

void UDP::_Socket::send(Net::IPv4::SockAddr addr, const void* data,
		size_t bytes) THROWS {
	Net::IPv4::UdpPacketBuffer out;
	out.setId(_id++);
	out.setSrcAddr(_addr.ip);
	out.setSrcPort(_addr.port);
	out.setDestAddr(addr.ip);
	out.setDestPort(addr.port);
	out.write(0, data, bytes);
	out.setDataSize(bytes);
	out.fillChecksum();
	_this->_ipv4->sendPacket(out);
}

void UDP::dispatchPacket(Net::IPv4::IpPacket& packet) THROWS {
	if (Net::IPv4::UdpPacket::isValid(packet)) {
		Net::IPv4::UdpPacket in = packet;
		Net::IPv4::SockAddr dst(in.getDestAddr(), in.getDestPort());
		_Socket* socket = _sockets.get(dst);
		if (socket) {
			Net::IPv4::SockAddr src(in.getSrcAddr(), in.getSrcPort());
			socket->_listener->onReceived(src, in.dataPtr(), in.getDataSize());
		}
	}
}

}
