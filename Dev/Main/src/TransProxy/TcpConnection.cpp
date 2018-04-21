#define LOG_TAG  "TcpConnection"

#include "Base/Debug.h"
#include "Base/Utils.h"
#include "TcpConnection.h"

namespace TransProxy {

TcpConnection::TcpConnection(TcpConnections* conns, IPv4* ipv4, uint32_t ip) :
		_conns(conns), _ipv4(ipv4), _serverListener(
		NULL), _listener(
		NULL), _id(1), _remoteWindowSize(0), _state(STATE_CLOSED), _retryCount(
				0), _localSeq(::random()), _remoteSeq(0), _needRecv(false), _needSend(
				false), _canRecv(false), _canSend(false), _needAck(false), _timerListener2(
				this), _timer1("ProtocolTcpConn1", this), _timer2(
				"ProtocolTcpConn2", &_timerListener2) {
	_addrs.remote.ip = ip;
	_inRing = Utils::RingBuffer::alloc(1400);
	_outRing = Utils::RingBuffer::alloc(1400);
}

TcpConnection::~TcpConnection() {
	_conns->remove(this);
	_timer1.clearTimeout();
	_timer2.clearTimeout();
	delete _inRing;
	delete _outRing;
}

void TcpConnection::sendPacket(int flags, size_t offset, size_t bytes,
		const void* data) THROWS {
	if (_needAck || bytes > 0) {
		flags |= Net::IPv4::TcpPacket::FLAG_ACK;
		_needAck = false;
	}
	Net::IPv4::TcpPacketBuffer out;
	out.setId(_id++);
	out.setSrcAddr(_addrs.local.ip);
	out.setSrcPort(_addrs.local.port);
	out.setDestAddr(_addrs.remote.ip);
	out.setDestPort(_addrs.remote.port);
	out.setFlags(flags);
	out.setSeq(_localSeq + offset);
	out.setAck(_remoteSeq);
	out.setWindowSize(_inRing->free());
	if (bytes > 0)
		out.write(0, data, bytes);
	out.setDataSize(bytes);
	out.fillChecksum();
	Utils::Log::d("sendPacket %s", out.toString().sz());
	_ipv4->sendPacket(out);
}

void TcpConnection::dispatchPacket(Net::IPv4::TcpPacket& in) THROWS {
	Utils::Log::d("dispatchPacket %s", in.toString().sz());
	_remoteWindowSize = in.getWindowSize();
	if (_state == STATE_LISTEN) {
		THROW_IF(!in.hasFlags(Net::IPv4::TcpPacket::FLAG_SYN),
				new Utils::Exception("LISTEN only accept SYN"));
		_addrs.remote = Net::IPv4::SockAddr(in.getSrcAddr(), in.getSrcPort());
		_remoteSeq = in.getSeq() + 1;
		_state = STATE_SYN_RECV;
		_retryCount = 0;
		_timer1.setTimeout(0);

	} else if (in.getSeq() == _remoteSeq) {
		if (_state == STATE_LAST_ACK) {
			if (in.hasFlags(Net::IPv4::TcpPacket::FLAG_ACK)
					&& in.getAck() == _localSeq + 1) {
				++_localSeq;
				_state = STATE_CLOSED;
				_timer1.setTimeout(0);
			}

		} else if (_state == STATE_FIN_WAIT_1) {
			bool fin = in.hasFlags(Net::IPv4::TcpPacket::FLAG_FIN);
			bool ack = in.hasFlags(Net::IPv4::TcpPacket::FLAG_ACK)
					&& in.getAck() == _localSeq + 1;
			if (ack) {
				++_localSeq;
				_state = STATE_FIN_WAIT_2;
				_timer1.setTimeout(3000);
			} else if (fin) {
				_state = STATE_CLOSING;
				_retryCount = 0;
				_timer1.setTimeout(0);
			} else if (fin && ack) {
				++_localSeq;
				sendPacket(Net::IPv4::TcpPacket::FLAG_ACK);
				_state = STATE_CLOSED;
				_timer1.setTimeout(0);
			}

		} else if (_state == STATE_FIN_WAIT_2) {
			if (in.hasFlags(Net::IPv4::TcpPacket::FLAG_FIN)) {
				sendPacket(Net::IPv4::TcpPacket::FLAG_ACK);
				_state = STATE_CLOSED;
				_timer1.setTimeout(0);
			}

		} else if (_state == STATE_CLOSING) {
			if (in.hasFlags(Net::IPv4::TcpPacket::FLAG_ACK)
					&& in.getAck() == _localSeq + 1) {
				++_localSeq;
				_state = STATE_CLOSED;
				_timer1.setTimeout(0);
			}

		} else if (in.hasFlags(Net::IPv4::TcpPacket::FLAG_FIN)) {
			++_remoteSeq;
			_state = STATE_LAST_ACK;
			_retryCount = 0;
			_timer1.setTimeout(0);

		} else if (_state == STATE_SYN_RECV) {
			if (in.hasFlags(Net::IPv4::TcpPacket::FLAG_ACK)
					&& in.getAck() == _localSeq + 1) {
				++_localSeq;
				_state = STATE_ESTABLISHED;
				Utils::Log::d("onTcpServerConnected");
				_listener = _serverListener->onTcpServerConnected(this);
			}

		} else if (_state == STATE_SYN_SENT) {
			if (in.hasFlags(Net::IPv4::TcpPacket::FLAG_SYN)
					&& in.hasFlags(Net::IPv4::TcpPacket::FLAG_ACK)
					&& in.getAck() == _localSeq + 1) {
				++_localSeq;
				sendPacket(Net::IPv4::TcpPacket::FLAG_ACK);
				_state = STATE_ESTABLISHED;
				Utils::Log::d("onConnected");
				_listener->onTcpConnected();
			}

		} else if (_state == STATE_ESTABLISHED) {
			if (in.hasFlags(Net::IPv4::TcpPacket::FLAG_ACK)) {
				size_t ack = in.getAck() - _localSeq;
				if (ack > 0 && ack < (1 << 30)) {
					Utils::Log::d("ACK %u bytes", ack);
					uint8_t buf[ack];
					_outRing->read(buf, ack);
					_localSeq += ack;
					if (_outRing->available() == 0) {
						_canSend = true;
						if (_needSend)
							_timer1.setTimeout(0);
					}
				}
			}
			size_t bytes = Utils::min(in.getDataSize(), _inRing->free());
			Utils::Log::d("Received %u bytes", bytes);
			if (bytes != 0 && bytes <= _inRing->free()) {
				_inRing->write(in.dataPtr(), bytes);
				_remoteSeq += bytes;
				if (in.hasFlags(Net::IPv4::TcpPacket::FLAG_PSH)
						|| _inRing->free() == 0) {
					_canRecv = true;
				}
				_needAck = true;
				_timer1.setTimeout(0);
			}
		}
	}
}

void TcpConnection::onTimeout() THROWS {
	if (_state == STATE_SYN_SENT) {
		if (_retryCount++ < 3) {
			sendPacket(Net::IPv4::TcpPacket::FLAG_SYN);
			_timer1.setTimeout(3000);
		} else {
			sendPacket(Net::IPv4::TcpPacket::FLAG_RST);
			_state = STATE_CLOSED;
			_timer1.setTimeout(0);
		}

	} else if (_state == STATE_SYN_RECV) {
		if (_retryCount++ < 3) {
			sendPacket(
					Net::IPv4::TcpPacket::FLAG_SYN
							| Net::IPv4::TcpPacket::FLAG_ACK);
			_timer1.setTimeout(3000);
		} else {
			sendPacket(Net::IPv4::TcpPacket::FLAG_RST);
			_state = STATE_CLOSED;
			_timer1.setTimeout(0);
		}

	} else if (_state == STATE_ESTABLISHED) {
		if (_needRecv && _canRecv) {
			_needRecv = false;
			_listener->onTcpToRecv();
		}
		if (_needSend && _canSend) {
			_needSend = false;
			_listener->onTcpToSend();
		}

	} else if (_state == STATE_LAST_ACK) {
		if (_retryCount++ < 3) {
			sendPacket(
					Net::IPv4::TcpPacket::FLAG_FIN
							| Net::IPv4::TcpPacket::FLAG_ACK);
			_timer1.setTimeout(3000);
		} else {
			sendPacket(Net::IPv4::TcpPacket::FLAG_RST);
			_state = STATE_CLOSED;
			_timer1.setTimeout(0);
		}

	} else if (_state == STATE_FIN_WAIT_1) {
		if (_retryCount++ < 3) {
			sendPacket(
					Net::IPv4::TcpPacket::FLAG_FIN
							| Net::IPv4::TcpPacket::FLAG_ACK);
			_timer1.setTimeout(3000);
		} else {
			sendPacket(Net::IPv4::TcpPacket::FLAG_RST);
			_state = STATE_CLOSED;
			_timer1.setTimeout(0);
		}

	} else if (_state == STATE_FIN_WAIT_2) {
		sendPacket(Net::IPv4::TcpPacket::FLAG_RST);
		_state = STATE_CLOSED;
		_timer1.setTimeout(0);

	} else if (_state == STATE_CLOSING) {
		if (_retryCount++ < 3) {
			sendPacket(Net::IPv4::TcpPacket::FLAG_ACK);
		} else {
			sendPacket(Net::IPv4::TcpPacket::FLAG_RST);
			_state = STATE_CLOSED;
			_timer1.setTimeout(0);
		}

	} else if (_state == STATE_CLOSED) {
		if (_listener)
			_listener->onTcpDisconnected();
		delete this;
	}
}

void TcpConnection::_send(size_t offset, size_t bytes) THROWS {
	size_t l = Utils::min<size_t>(offset + bytes, _remoteWindowSize);
	if (l > offset) {
		l -= offset;
		uint8_t buf[1024];
		while (l > sizeof(buf)) {
			_outRing->peek(offset, sizeof(buf), buf);
			sendPacket(0, offset, sizeof(buf), buf);
			offset += sizeof(buf);
			l -= sizeof(buf);
		}
		_outRing->peek(offset, l, buf);
		sendPacket(Net::IPv4::TcpPacket::FLAG_PSH, offset, l, buf);
	}
	if (_needAck)
		sendPacket(0);
}

void TcpConnection::_TimerListener2::onTimeout() THROWS {
	if (_this->_state == STATE_ESTABLISHED) {
		_this->_send(0, _this->_outRing->available());
		_this->_timer2.setTimeout(3000);
	}
}

void TcpConnection::accept(Net::IPv4::SockAddrPair addrs,
		Net::TcpServerListener* listener) THROWS {
	THROW_IF(_state != STATE_CLOSED,
			new Utils::Exception("Listen at non-closed state"));
	Utils::Log::d("accept %s", addrs.toString().sz());
	_addrs = addrs;
	_conns->add(this);
	_serverListener = listener;
	_state = STATE_LISTEN;
}

void TcpConnection::connect(Net::IPv4::SockAddr addr) THROWS {
	THROW_IF(_state != STATE_CLOSED,
			new Utils::Exception("Connect at non-closed state"));
	uint32_t ip = _addrs.remote.ip;
	for (uint16_t port = 1024; port < 65536; ++port) {
		Net::IPv4::SockAddrPair addrs(Net::IPv4::SockAddr(ip, port), addr);
		if (_conns->get(addrs) == NULL) {
			Utils::Log::d("connect %s", addrs.toString().sz());
			_addrs = addrs;
			_conns->add(this);
			_state = STATE_SYN_SENT;
			_retryCount = 0;
			_timer1.setTimeout(0);
			return;
		}
	}
	THROW(new Utils::Exception("No free port to bind"));
}

void TcpConnection::close() THROWS {
	_listener = NULL;
	if (_state != STATE_FIN_WAIT_1 && _state != STATE_FIN_WAIT_2
			&& _state != STATE_CLOSING) {
		_state = STATE_FIN_WAIT_1;
		_retryCount = 0;
		_timer1.setTimeout(0);
	}
}

void TcpConnection::waitToRecv() {
	Utils::Log::d("waitCanRecv");
	if (_state == STATE_ESTABLISHED) {
		_needRecv = true;
		if (_canRecv)
			_timer1.setTimeout(0);
	}
}

void TcpConnection::waitToSend() {
	Utils::Log::d("waitCanSend");
	if (_state == STATE_ESTABLISHED) {
		_needSend = true;
		if (_canSend)
			_timer1.setTimeout(0);
	}
}

size_t TcpConnection::peek(void* data, size_t bytes) THROWS {
	Utils::Log::d("==> peek %u", bytes);
	size_t r;
	if (_state == STATE_ESTABLISHED)
		r = _inRing->peek(data, bytes);
	else
		r = 0;
	Utils::Log::d("%u <== peek", r);
	return r;
}

size_t TcpConnection::recv(void* data, size_t bytes) THROWS {
	Utils::Log::d("==> recv %u", bytes);
	size_t r;
	if (_state == STATE_ESTABLISHED) {
		r = _inRing->read(data, bytes);
		if (_inRing->available() == 0)
			_canRecv = false;
	} else {
		r = 0;
	}
	Utils::Log::d("%u <== recv", r);
	Utils::Log::dump(data, r);
	return r;
}

size_t TcpConnection::send(const void* data, size_t bytes) THROWS {
	Utils::Log::d("==> send %u", bytes);
	size_t r;
	if (_state == STATE_ESTABLISHED) {
		size_t offset = _outRing->available();
		r = _outRing->write(data, bytes);
		if (_outRing->free() == 0)
			_canSend = false;
		if (r) {
			_send(offset, r);
			_timer2.setTimeout(3000);
		}
	} else {
		r = 0;
	}
	Utils::Log::d("%u <== send", r);
	return r;
}

}
