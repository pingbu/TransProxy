#include "IPv4.h"
#include "TcpConnection.h"

#pragma once

namespace Net {

class ProxySocksConnection: public TcpConnection {
	TcpConnection* _conn;
	TcpConnectionListener* _listener;
	bool _ready;

	struct _: TcpConnectionListener {
		ProxySocksConnection* _this;
		_(ProxySocksConnection* thiz) :
				_this(thiz) {
		}
		void onTcpConnected() THROWS;
		void onTcpDisconnected() THROWS {
			_this->_listener->onTcpDisconnected();
		}
		void onTcpToRecv() THROWS;
		void onTcpToSend() THROWS {
			ASSERT(_this->_ready);
			_this->_listener->onTcpToSend();
		}
		void onTcpError(Utils::Exception* e) THROWS {
			_this->_listener->onTcpError(e);
		}
	} _tcpClientListener;

public:
	ProxySocksConnection(TcpConnection* conn, TcpConnectionListener* listener =
	NULL) :
			_conn(conn), _listener(listener), _ready(false), _tcpClientListener(
					this) THROWS {
		listener = _conn->setListener(&_tcpClientListener);
		if (_listener == NULL)
			_listener = listener;
	}

	TcpConnectionListener* setListener(TcpConnectionListener* listener) {
		TcpConnectionListener* prevListener = _listener;
		_listener = listener;
		return prevListener;
	}

	void connect(IPv4::SockAddr addr) THROWS {
		_conn->connect(addr);
	}
	void accept(int server) THROWS {
		THROW(new Utils::Exception("Unsupported"));
	}

	IPv4::SockAddr getLocalAddr() const {
		return _conn->getLocalAddr();
	}
	IPv4::SockAddr getRemoteAddr() const {
		return _conn->getRemoteAddr();
	}

	void close() {
		delete this;
	}

	void waitToRecv() {
		ASSERT(_ready);
		_conn->waitToRecv();
	}
	void waitToSend() {
		ASSERT(_ready);
		_conn->waitToSend();
	}

	size_t peek(void* data, size_t bytes) THROWS {
		ASSERT(_ready);
		return _conn->peek(data, bytes);
	}
	size_t recv(void* data, size_t bytes) THROWS {
		ASSERT(_ready);
		return _conn->recv(data, bytes);
	}
	size_t send(const void* data, size_t bytes) THROWS {
		ASSERT(_ready);
		return _conn->send(data, bytes);
	}
};

}
