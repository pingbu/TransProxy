#define LOG_TAG "TCP"

#include "Base/Debug.h"
#include "Base/Utils.h"
#include "TCP.h"

namespace TransProxy {

void TCP::_Server::dispatchPacket(Net::IPv4::TcpPacket& in) THROWS {
	if (in.hasFlags(Net::IPv4::TcpPacket::FLAG_SYN)) {
		TcpConnection* conn = new TcpConnection(&_this->_connections,
				_this->_ipv4);
		conn->accept(
				Net::IPv4::SockAddrPair(
						Net::IPv4::SockAddr(in.getSrcAddr(), in.getSrcPort()),
						_addr), _listener);
		conn->dispatchPacket(in);
	}
}

void TCP::dispatchPacket(Net::IPv4::IpPacket& packet) THROWS {
	if (Net::IPv4::TcpPacket::isValid(packet)) {
		Net::IPv4::TcpPacket in = packet;
		Net::IPv4::SockAddr src(in.getSrcAddr(), in.getSrcPort());
		Net::IPv4::SockAddr dst(in.getDestAddr(), in.getDestPort());
		Net::IPv4::SockAddrPair addr(src, dst);
		TcpConnection* conn = _connections.get(addr);
		if (conn) {
			conn->dispatchPacket(in);
		} else {
			_Server* server = _servers.get(dst);
			if (server) {
				server->dispatchPacket(in);
			} else {
				_ServerIP* serverIP = _serverIPs.get(dst.ip);
				if (serverIP) {
					in.setFlags(Net::IPv4::TcpPacket::FLAG_RST);
					in.setSrcAddr(dst.ip);
					in.setDestAddr(src.ip);
					in.setSrcPort(dst.port);
					in.setDestPort(src.port);
					_ipv4->sendPacket(in);
				}
			}
		}
	}
}

}
