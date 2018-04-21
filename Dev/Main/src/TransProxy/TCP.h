#include "Base/Debug.h"
#include "Base/Utils.h"
#include "IPv4.h"
#include "TcpConnection.h"

#pragma once

namespace TransProxy {

class TCP: public IPv4Protocol {
public:
	struct Factory: Net::TcpServerFactory, Net::TcpClientFactory {
	};

private:
	struct _ServerIP: Utils::MapItem<uint32_t> {
		uint32_t ip;
		size_t count;
		_ServerIP(uint32_t ip) :
				ip(ip), count(1) {
		}
		uint32_t getKey() const {
			return ip;
		}
		Utils::String getKeyString() const {
			return Utils::String::format("%x", ip);
		}
	};

	struct _Server: Net::TcpServer, Utils::MapItem<Net::IPv4::SockAddr> {
		TCP* _this;
		Net::IPv4::SockAddr _addr;
		Net::TcpServerListener* _listener;
		uint16_t _id;
		_Server(TCP* thiz, Net::IPv4::SockAddr addr,
				Net::TcpServerListener* listener) :
				_this(thiz), _addr(addr), _listener(listener), _id(1) {
			_this->_servers.add(this);
			_ServerIP* serverIP = _this->_serverIPs.get(addr.ip);
			if (serverIP)
				++serverIP->count;
			else
				_this->_serverIPs.add(new _ServerIP(addr.ip));
		}
		~_Server() {
			_this->_servers.remove(this);
			_ServerIP* serverIP = _this->_serverIPs.get(_addr.ip);
			ASSERT(serverIP);
			if (--serverIP->count == 0)
				_this->_serverIPs.remove(serverIP);
		}

		void dispatchPacket(Net::IPv4::TcpPacket& packet) THROWS;

		// MapItem
		Net::IPv4::SockAddr getKey() const {
			return _addr;
		}
		Utils::String getKeyString() const {
			return _addr.toString();
		}

		// Net::TcpServer
		uint16_t getPort() const {
			return _addr.port;
		}
	};

	struct _Factory: Factory {
		TCP* _this;
		uint32_t _ip;
		_Factory(TCP* thiz, uint32_t ip) :
				_this(thiz), _ip(ip) {
		}
		Net::TcpServer* tcp_listen(uint16_t port,
				Net::TcpServerListener* listener) THROWS {
			return _this->listen(Net::IPv4::SockAddr(_ip, port), listener);
		}
		Net::TcpConnection* tcp_open(Net::TcpConnectionListener* listener)
				THROWS {
			return _this->open(_ip, listener);
		}
	};

	IPv4* _ipv4;
	Utils::Map<uint32_t, _ServerIP> _serverIPs;
	Utils::Map<Net::IPv4::SockAddr, _Server> _servers;
	Utils::Map<Net::IPv4::SockAddrPair, TcpConnection> _connections;

public:
	TCP(IPv4* ipv4) :
			_ipv4(ipv4) THROWS {
		Utils::Log::i("Protocol TCP initializing...");
	}
	virtual ~TCP() {
		Utils::Log::e("~TCP");
	}

	// IPv4Protocol
	void dispatchPacket(Net::IPv4::IpPacket& packet) THROWS;

	Factory* bind(uint32_t ip) {
		return new _Factory(this, ip);
	}
	Net::TcpServer* listen(Net::IPv4::SockAddr addr,
			Net::TcpServerListener* listener) {
		return new _Server(this, addr, listener);
	}
	Net::TcpConnection* open(uint32_t ip,
			Net::TcpConnectionListener* listener) {
		return new TcpConnection(&_connections, _ipv4, ip);
	}
};

}
