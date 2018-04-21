#define LOG_TAG  "TransTCP"

#include <limits.h>
#include "Base/Debug.h"
#include "Base/Utils.h"
#include "DomainResolver.h"
#include "TransTCP.h"

namespace TransProxy {

const Net::IPv4::SockAddrPair& TransTCP::_ConnectionByAddrPair::getKey() const {
	return (*this)->_addrPair;
}

Net::IPv4::SockAddr TransTCP::_ConnectionByAgent::getKey() const {
	return (*this)->_agent;
}

void TransTCP::_Connection::_sendPacket(_From from, int flags, uint32_t seq,
		uint32_t ack, uint16_t windowSize, const void* data, size_t bytes)
				THROWS {
	Net::IPv4::TcpPacketBuffer out;
	out.setFlags(flags | Net::IPv4::TcpPacket::FLAG_ACK);
	out.setSeq(seq);
	out.setAck(ack);
	out.setWindowSize(windowSize);
	if (bytes > 0) {
		out.write(0, data, bytes);
		out.setDataSize(bytes);
	}
	_sendPacket(from, out);
}

void TransTCP::_Connection::_sendPacket(_From from, Net::IPv4::TcpPacket& out)
		THROWS {
	if (from == FROM_CLIENT) {
		out.setSrcSockAddr(_agent);
		out.setDestSockAddr(_proxy);
	} else {
		out.setSrcSockAddr(_addrPair.local);
		out.setDestSockAddr(_addrPair.remote);
	}
	out.fillChecksum();
	Utils::Log::d("_sendPacket %s", out.toString().sz());
	_this->_ipv4->sendPacket(out);
}

void TransTCP::_Connection::_transferData(_From from,
		Net::IPv4::TcpPacket& packet) THROWS {
	uint32_t seq = packet.getSeq();
	uint32_t ack = packet.getAck();
	bool ACK = packet.hasFlags(Net::IPv4::TcpPacket::FLAG_ACK);
	if (from == FROM_CLIENT) {
		packet.setSeq(seq + _proxyOutTotal);
		packet.setAck(ack + _proxyInTotal);
		if (ACK) {
			size_t downBytes = ack - _proxySeq - _downBytes;
			if (downBytes > 0 && downBytes <= INT_MAX) {
				_downBytes += downBytes;
				_this->_totalDownBytes += downBytes;
			}
		}
	} else {
		packet.setSeq(seq -= _proxyInTotal);
		packet.setAck(ack -= _proxyOutTotal);
		if (ACK) {
			size_t upBytes = ack - _clientSeq - _upBytes;
			if (upBytes > 0 && upBytes <= INT_MAX) {
				_upBytes += upBytes;
				_this->_totalUpBytes += upBytes;
			}
		}
	}
	_sendPacket(from, packet);
}

void TransTCP::_Connection::_transferSYN1(_From from,
		Net::IPv4::TcpPacket& packet) THROWS {
	if (from == FROM_CLIENT
			&& packet.hasFlags(Net::IPv4::TcpPacket::FLAG_SYN)) {
		_clientSeq = packet.getSeq() + 1;
		_clientWindowSize = packet.getWindowSize();
		packet.setWindowSize(0);
		_sendPacket(from, packet);
		_state = STATE_SYN_SENT;
		_timer.setTimeout(3000);
	}
}

void TransTCP::_Connection::_transferSYN2(_From from,
		Net::IPv4::TcpPacket& packet) THROWS {
	if (from == FROM_PROXY && packet.hasFlags(Net::IPv4::TcpPacket::FLAG_SYN)
			&& packet.hasFlags(Net::IPv4::TcpPacket::FLAG_ACK)
			&& packet.getAck() == _clientSeq) {
		_proxySeq = packet.getSeq() + 1;
		packet.setWindowSize(0);
		_sendPacket(from, packet);
		_state = STATE_SYN_RECEIVED;
		_timer.setTimeout(3000);
	}
}

void TransTCP::_Connection::_transferSYN3(_From from,
		Net::IPv4::TcpPacket& packet) THROWS {
	if (from == FROM_CLIENT && packet.hasFlags(Net::IPv4::TcpPacket::FLAG_ACK)
			&& packet.getAck() == _proxySeq) {
		packet.setWindowSize(0);
		_sendPacket(from, packet);
		_auth = _this->_authBuilder->createInstance(_hostname.sz(),
				_addrPair.local.port, this);
		_state = STATE_AUTH;
		_retryCount = 0;
		_timer.setTimeout(0);
	}
}

void TransTCP::_Connection::_observeFIN(_From from,
		Net::IPv4::TcpPacket& packet) THROWS {
	if (packet.hasFlags(Net::IPv4::TcpPacket::FLAG_FIN)) {
		if (from == FROM_CLIENT)
			_clientFin = true;
		else
			_proxyFin = true;
	}
}

void TransTCP::_Connection::dispatchPacket(_From from,
		Net::IPv4::TcpPacket& packet) THROWS {
	Utils::Log::d("==> (%u) dispatchPacket %s", _state, packet.toString().sz());

	if (_state == STATE_CLOSED) {
		_transferSYN1(from, packet);

	} else if (_state == STATE_SYN_SENT) {
		_transferSYN1(from, packet);
		_transferSYN2(from, packet);

	} else if (_state == STATE_SYN_RECEIVED) {
		_transferSYN2(from, packet);
		_transferSYN3(from, packet);

	} else if (_state == STATE_AUTH) {
		_authDispatchPacket(from, packet);

	} else if (_state == STATE_ESTABLISHING || _state == STATE_ESTABLISHED) {
		if (_state == STATE_ESTABLISHING)
			_establishingDispatchPacket(from, packet);
		_transferData(from, packet);
		_observeFIN(from, packet);

		_timer.clearTimeout();
		if (_clientFin && _proxyFin) {
			_state = STATE_FIN_WAIT;
			_timer.setTimeout(3000);
		} else {
			_timer.setTimeout(15 * 60 * 1000);
		}

	} else if (_state == STATE_FIN_WAIT) {
		_transferData(from, packet);

	} else if (_state == STATE_CLOSING) {
		_closingDispatchPacket(from, packet);
	}

	Utils::Log::d("<== (%u) dispatchPacket", _state);
}

void TransTCP::_Connection::_authSendRequest() THROWS {
	_auth->sendAuthRequest();
}

void TransTCP::_Connection::sendAuthRequest(const void* data, size_t bytes,
		size_t bufferSize) THROWS {
	_sendPacket(FROM_CLIENT, Net::IPv4::TcpPacket::FLAG_PSH,
			_clientSeq + _proxyOutTotal, _proxySeq + _proxyInTotal, bufferSize,
			data, bytes);
}

void TransTCP::_Connection::commitAuthRequest(size_t bytes) {
	_proxyOutTotal += bytes;
}

void TransTCP::_Connection::_authDispatchPacket(_From from,
		Net::IPv4::TcpPacket& packet) THROWS {
	Utils::Log::d("==> _authDispatchPacket");
	if (from == FROM_PROXY && packet.getSeq() == _proxySeq + _proxyInTotal) {
		size_t bytes = packet.getDataSize();
		_proxyInTotal += bytes;
		int r = _auth->onAuthResponse(packet.dataPtr(), bytes);
		if (r >= 0) {
			delete _auth;
			_auth = NULL;
			if (r) {
				_proxyWindowSize = packet.getWindowSize();
				_state = STATE_ESTABLISHING;
				_retryCount = 0;
				_timer.setTimeout(0);
			} else {
				Utils::Log::e("FAILED to connect %s --> %s:%u",
						_addrPair.remote.toString().sz(), _hostname.sz(),
						_addrPair.local.port);
				_close();
			}
		}
	}
	Utils::Log::d("<== _authDispatchPacket");
}

void TransTCP::_Connection::_establishingSendRequest() THROWS {
	_sendPacket(FROM_PROXY, 0, _proxySeq, _clientSeq, _proxyWindowSize);
	_sendPacket(FROM_CLIENT, 0, _clientSeq + _proxyOutTotal,
			_proxySeq + _proxyInTotal, _clientWindowSize);
}

void TransTCP::_Connection::_establishingDispatchPacket(_From from,
		Net::IPv4::TcpPacket& packet) THROWS {
	Utils::Log::i("Connected %s --> %s:%u", _addrPair.remote.toString().sz(),
			_hostname.sz(), _addrPair.local.port);
	_state = STATE_ESTABLISHED;
}

void TransTCP::_Connection::_closingSendRequest() THROWS {
	if (!_clientFin)
		_sendPacket(FROM_PROXY, Net::IPv4::TcpPacket::FLAG_FIN, _proxySeq,
				_clientSeq, _proxyWindowSize);
	if (!_proxyFin)
		_sendPacket(FROM_CLIENT, Net::IPv4::TcpPacket::FLAG_FIN,
				_clientSeq + _proxyOutTotal, _proxySeq + _proxyInTotal,
				_clientWindowSize);
}

void TransTCP::_Connection::_closingDispatchPacket(_From from,
		Net::IPv4::TcpPacket& packet) THROWS {
	if (packet.hasFlags(Net::IPv4::TcpPacket::FLAG_ACK)) {
		uint32_t ack = packet.getAck();
		if (from == FROM_CLIENT) {
			if (ack == _proxySeq + 1)
				_clientFin = true;
		} else {
			if (ack == _clientSeq + _proxyOutTotal + 1)
				_proxyFin = true;
		}
		if (_clientFin && _proxyFin)
			_closed();
	}
}

void TransTCP::_Connection::_close() {
	_state = STATE_CLOSING;
	_retryCount = 0;
	_timer.setTimeout(0);
}

void TransTCP::_Connection::_closed() {
	_state = STATE_CLOSED;
	_timer.setTimeout(0);
}

void TransTCP::_Connection::_try(void (_Connection::*fn)(),
		void (_Connection::*timeout)()) THROWS {
	if (_retryCount++ < 3) {
		(this->*fn)();
		_timer.setTimeout(3000);
	} else {
		(this->*timeout)();
	}
}

void TransTCP::_Connection::onTimeout() THROWS {
	if (_state == STATE_SYN_SENT) {
		_close();

	} else if (_state == STATE_SYN_RECEIVED) {
		_close();

	} else if (_state == STATE_AUTH) {
		_try(&_Connection::_authSendRequest, &_Connection::_close);

	} else if (_state == STATE_ESTABLISHING) {
		_try(&_Connection::_establishingSendRequest, &_Connection::_close);

	} else if (_state == STATE_ESTABLISHED) {
		_close();

	} else if (_state == STATE_FIN_WAIT) {
		_closed();

	} else if (_state == STATE_CLOSING) {
		_try(&_Connection::_closingSendRequest, &_Connection::_closed);

	} else if (_state == STATE_CLOSED) {
		Utils::Log::i("Disconnected %s --> %s:%u",
				_addrPair.remote.toString().sz(), _hostname.sz(),
				_addrPair.local.port);
		delete this;
	}
}

Net::IPv4::SockAddr TransTCP::_allocAgentAddress() {
	if (_agentAddr.port++ == AGENT_PORT_MAX) {
		_agentAddr.port = AGENT_PORT_MIN;
		++_agentAddr.ip;
	}
	return _agentAddr;
}

void TransTCP::dispatchPacket(Net::IPv4::IpPacket& packet) THROWS {
	if (Net::IPv4::TcpPacket::isValid(packet)) {
		Net::IPv4::TcpPacket in = packet;
		Net::IPv4::SockAddr src(in.getSrcAddr(), in.getSrcPort());
		Net::IPv4::SockAddr dst(in.getDestAddr(), in.getDestPort());
		Net::IPv4::SockAddrPair addr(src, dst);

		_Connection* conn;
		const char* hostname;

		if ((conn = *_addrPairMap.get(addr))) {
			conn->dispatchPacket(_Connection::FROM_CLIENT, in);

		} else if ((conn = *_agentMap.get(addr.local))) {
			conn->dispatchPacket(_Connection::FROM_PROXY, in);

		} else if ((hostname = _domainResolver->ddns(addr.local.ip))) {
			if (in.hasFlags(Net::IPv4::TcpPacket::FLAG_SYN)
					&& !in.hasFlags(Net::IPv4::TcpPacket::FLAG_ACK)) {
				conn = new _Connection(this, addr.remote, addr.local,
						_allocAgentAddress(), _proxy, hostname);
				_maxConnCount = Utils::max(_maxConnCount, _addrPairMap.size());
				conn->dispatchPacket(_Connection::FROM_CLIENT, in);
			} else {
				in.setFlags(Net::IPv4::TcpPacket::FLAG_RST);
				uint32_t srcAddr = in.getSrcAddr();
				uint32_t dstAddr = in.getDestAddr();
				uint16_t srcPort = in.getSrcPort();
				uint16_t dstPort = in.getDestPort();
				in.setSrcAddr(dstAddr);
				in.setDestAddr(srcAddr);
				in.setSrcPort(dstPort);
				in.setDestPort(srcPort);
				_ipv4->sendPacket(in);
			}
		}
	}
}

class IpSetItem: public Utils::MapItem<uint32_t> {
	uint32_t _ip;
public:
	IpSetItem(uint32_t ip) {
		_ip = ip;
	}
	operator uint32_t() const {
		return _ip;
	}
	uint32_t getKey() const {
		return _ip;
	}
	Utils::String getKeyString() const {
		return Net::IPv4::ntoa(_ip);
	}
};

bool TransTCP::onHttpRequest(Net::HttpRequest& request,
		Utils::JSONObject& response) THROWS {
	Utils::String path = request.getPath();
	if (path == "/tcpconn.json") {
		uint32_t client = request.getRemoteAddr().ip;
		Utils::Map<uint32_t, IpSetItem> ips;
		ips.add(new IpSetItem(client));
		const char* client_ = request.getQueryString("client");
		if (client_) {
			uint32_t client__ = Net::IPv4::aton(client_);
			if (client__ != client) {
				client = client__;
				ips.add(new IpSetItem(client));
			}
		}
		for (_ConnectionByAddrPair* item = _addrPairMap.min(); item; item =
				_addrPairMap.bigger(item)) {
			uint32_t ip = (*item)->_addrPair.remote.ip;
			if (!ips.get(ip))
				ips.add(new IpSetItem(ip));
		}

		Utils::JSONArray* clients = new Utils::JSONArray();
		for (IpSetItem* item = ips.min(); item; item = ips.bigger(item))
			clients->put(Net::IPv4::ntoa(*item));

		Utils::JSONArray* conns = new Utils::JSONArray();
		for (_ConnectionByAddrPair* item = _addrPairMap.min(); item; item =
				_addrPairMap.bigger(item))
			if ((*item)->_addrPair.remote.ip == client) {
				Utils::JSONObject* conn = new Utils::JSONObject();
				conns->put(conn);

				Utils::String server = Utils::String::format("%s:%u",
						(*item)->_hostname.sz(), (*item)->_addrPair.local.port);
				conn->put("Server", server.sz());

				conn->put("UpBytes", Utils::formatSize((*item)->_upBytes).sz());
				conn->put("DownBytes",
						Utils::formatSize((*item)->_downBytes).sz());

				unsigned t = ::time(NULL) - (*item)->_time;
				conn->put("ConnTime", Utils::formatTimeSpan(t).sz());

				if ((*item)->_state == _Connection::STATE_SYN_SENT
						|| (*item)->_state == _Connection::STATE_SYN_RECEIVED) {
					conn->put("State", "Connecting");
				} else if ((*item)->_state == _Connection::STATE_AUTH) {
					conn->put("State", "Authorizing");
				} else if ((*item)->_state == _Connection::STATE_ESTABLISHING
						|| (*item)->_state == _Connection::STATE_ESTABLISHED) {
					conn->put("State", "Connected");
				} else if ((*item)->_state == _Connection::STATE_FIN_WAIT
						|| (*item)->_state == _Connection::STATE_CLOSING) {
					conn->put("State", "Closing");
				} else if ((*item)->_state == _Connection::STATE_CLOSED) {
					conn->put("State", "Closed");
				} else {
					Utils::String st = Utils::String::format("%u",
							(*item)->_state);
					conn->put("State", st.sz());
				}
			}

		response.put("Status", 0);
		response.put("Message", "OK");
		response.put("Clients", clients);
		response.put("CurrentClient", Net::IPv4::ntoa(client));
		response.put("Connections", conns);
		return true;
	}
	return HttpService::onHttpRequest(request, response);
}

}
