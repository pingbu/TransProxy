#include <string.h>
#include <time.h>
#include "Base/Debug.h"
#include "Base/Utils.h"
#include "DomainResolver.h"
#include "TcpConnection.h"
#include "ProxyAuth.h"
#include "ProxyAuthHTTP.h"
#include "ProxyAuthSock5.h"
#include "HTTP.h"
#include "IPv4.h"

#pragma once

namespace TransProxy {

class TransTCP: public IPv4Protocol,
		public HttpService,
		public DomainResolver::Rules {
	struct _Connection;

	enum {
		AGENT_PORT_MIN = 1025, AGENT_PORT_MAX = 65500
	};

	struct _ConnectionByAddrPair: Utils::MapItemPtr<
			const Net::IPv4::SockAddrPair&, _Connection> {
		_ConnectionByAddrPair(_Connection* p) :
				Utils::MapItemPtr<const Net::IPv4::SockAddrPair&, _Connection>(
						p) {
		}
		const Net::IPv4::SockAddrPair& getKey() const;
		Utils::String getKeyString() const {
			return getKey().toString();
		}
	};

	struct _ConnectionByAgent: Utils::MapItemPtr<Net::IPv4::SockAddr,
			_Connection> {
		_ConnectionByAgent(_Connection* p) :
				Utils::MapItemPtr<Net::IPv4::SockAddr, _Connection>(p) {
		}
		Net::IPv4::SockAddr getKey() const;
		Utils::String getKeyString() const {
			return getKey().toString();
		}
	};

	struct _Connection: Utils::TimerListener, ProxyAuthListener {
		enum _From {
			FROM_CLIENT, FROM_PROXY
		};
		enum _State {
			STATE_CLOSED,
			STATE_SYN_SENT,
			STATE_SYN_RECEIVED,
			STATE_AUTH,
			STATE_ESTABLISHING,
			STATE_ESTABLISHED,
			STATE_FIN_WAIT,
			STATE_CLOSING
		};
		TransTCP* _this;
		time_t _time;
		Net::IPv4::SockAddrPair _addrPair;
		Net::IPv4::SockAddr _agent, _proxy;
		Utils::String _hostname;
		_ConnectionByAddrPair _addrPairItem;
		_ConnectionByAgent _agentItem;
		Utils::Timer _timer;
		_State _state;
		int _retryCount;
		uint32_t _clientSeq, _proxySeq;
		uint16_t _clientWindowSize, _proxyWindowSize;
		ProxyAuth* _auth;
		size_t _proxyOutTotal, _proxyInTotal, _upBytes, _downBytes;
		bool _clientEstablished, _proxyEstablished, _clientFin, _proxyFin;

		_Connection(TransTCP* thiz, Net::IPv4::SockAddr client,
				Net::IPv4::SockAddr server, Net::IPv4::SockAddr agent,
				Net::IPv4::SockAddr proxy, const char* hostname) :
				_this(thiz), _time(::time(NULL)), _addrPair(client, server), _agent(
						agent), _proxy(proxy), _hostname(hostname), _addrPairItem(
						this), _agentItem(this), _timer("TransProxyConnection",
						this), _state(STATE_CLOSED), _retryCount(0), _clientSeq(
						0), _proxySeq(0), _clientWindowSize(0), _proxyWindowSize(
						0), _auth(NULL), _proxyOutTotal(0), _proxyInTotal(0), _upBytes(
						0), _downBytes(0), _clientEstablished(false), _proxyEstablished(
						false), _clientFin(false), _proxyFin(false) {
			_this->_addrPairMap.add(&_addrPairItem);
			_this->_agentMap.add(&_agentItem);
		}
		~_Connection() {
			_this->_addrPairMap.remove(&_addrPairItem);
			_this->_agentMap.remove(&_agentItem);
			if (_auth)
				delete _auth;
		}

		void _sendPacket(_From from, int flags, uint32_t seq, uint32_t ack,
				uint16_t windowSize, const void* data = NULL, size_t bytes = 0)
						THROWS;
		void _sendPacket(_From from, Net::IPv4::TcpPacket& packet) THROWS;

		void _transferData(_From from, Net::IPv4::TcpPacket& packet) THROWS;
		void _transferSYN1(_From from, Net::IPv4::TcpPacket& packet) THROWS;
		void _transferSYN2(_From from, Net::IPv4::TcpPacket& packet) THROWS;
		void _transferSYN3(_From from, Net::IPv4::TcpPacket& packet) THROWS;
		void _observeFIN(_From from, Net::IPv4::TcpPacket& packet) THROWS;
		void dispatchPacket(_From from, Net::IPv4::TcpPacket& packet) THROWS;

		void _authSendRequest() THROWS;
		void _authDispatchPacket(_From from, Net::IPv4::TcpPacket& packet)
				THROWS;

		void _establishingSendRequest() THROWS;
		void _establishingDispatchPacket(_From from,
				Net::IPv4::TcpPacket& packet) THROWS;

		void _closingSendRequest() THROWS;
		void _closingDispatchPacket(_From from, Net::IPv4::TcpPacket& packet)
				THROWS;

		void _try(void (_Connection::*fn)(), void (_Connection::*timeout)())
				THROWS;
		void _close();
		void _closed();

		// Utils::TimerListener
		void onTimeout() THROWS;
		void onTimerError(Utils::Exception* e) THROWS {
			THROW(e);
		}

		// TransProxyAuthListener
		void sendAuthRequest(const void* data, size_t bytes, size_t bufferSize)
				THROWS;
		void commitAuthRequest(size_t bytes);
	};

	friend struct _Connection;

	IPv4* _ipv4;
	Net::IPv4::SockAddr _agentAddr;
	DomainResolver* _domainResolver;
	Net::IPv4::SockAddr _proxy;
	ProxyAuthBuilder* _authBuilder;
	Utils::Map<const Net::IPv4::SockAddrPair&, _ConnectionByAddrPair> _addrPairMap;
	Utils::Map<Net::IPv4::SockAddr, _ConnectionByAgent> _agentMap;
	uint64_t _totalUpBytes, _totalDownBytes;
	size_t _maxConnCount;

	Net::IPv4::SockAddr _allocAgentAddress();

public:
	TransTCP(IPv4* ipv4, const char* agentIpBase,
			DomainResolver* domainResolver, const char* proxy,
			const char* clientIP) :
			_ipv4(ipv4), _agentAddr(Net::IPv4::aton(agentIpBase),
					AGENT_PORT_MIN - 1), _domainResolver(domainResolver), _totalUpBytes(
					0), _totalDownBytes(0), _maxConnCount(0) THROWS {
		Utils::Log::i("TransTCP initializing...");
		const char* p = ::strstr(proxy, "://");
		THROW_IF(p == NULL, new Utils::Exception("Invalid proxy url!"));
		_proxy = Net::IPv4::SockAddr(p + 3);
		if (::strncmp(proxy, "http://", 7) == 0) {
			_authBuilder = new ProxyAuthHTTP::Builder();
		} else if (::strncmp(proxy, "sock://", 7) == 0
				|| ::strncmp(proxy, "socks://", 8) == 0
				|| ::strncmp(proxy, "sock5://", 8) == 0) {
			_authBuilder = new ProxyAuthSock5::Builder();
		} else {
			THROW(new Utils::Exception("Only HTTP or SOCK5 proxy supported!"));
		}
		if ((_proxy.ip >> 24) == 127)
			_proxy.ip = Net::IPv4::aton(clientIP);
	}
	virtual ~TransTCP() {
		Utils::Log::e("~TransTCP");
	}

	size_t getConnectionCount() const {
		return _addrPairMap.size();
	}
	size_t getMaxConnectionCount() const {
		return _maxConnCount;
	}
	uint64_t getTotalUpBytes() const {
		return _totalUpBytes;
	}
	uint64_t getTotalDownBytes() const {
		return _totalDownBytes;
	}

	// IPv4Protocol
	void dispatchPacket(Net::IPv4::IpPacket& packet) THROWS;

	// DomainResolver::Rules
	bool acceptProxy(uint32_t client, const char* hostname) const {
		return false;
	}
	bool denyProxy(uint32_t client, const char* hostname) const {
		return client == _proxy.ip;
	}

	// HttpService
	bool onHttpRequest(Net::HttpRequest& request, Utils::JSONObject& response)
			THROWS;
};

}
