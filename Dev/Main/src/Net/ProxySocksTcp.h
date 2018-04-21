#include "Base/Utils.h"
#include "ProxySocksConnection.h"
#include "TcpConnection.h"

#pragma once

namespace Net {

class ProxySocksTcp: public TcpConnection {
	ProxySocksConnection* _conn;
	TcpConnectionListener* _listener;
	IPv4::SockAddr _proxyAddr, _remoteAddr;
	bool _ready;

	struct _: TcpConnectionListener {
		ProxySocksTcp* _this;
		_(ProxySocksTcp* thiz) :
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

	void _close() {
		if (_conn) {
			_conn->close();
			_conn = NULL;
		}
	}

protected:
	virtual ~ProxySocksTcp() {
	}

public:
	ProxySocksTcp(TcpConnection* conn, IPv4::SockAddr proxyAddr,
			TcpConnectionListener* listener = NULL) :
			_conn(new ProxySocksConnection(conn, &_tcpClientListener)), _listener(
					listener), _proxyAddr(proxyAddr), _ready(false), _tcpClientListener(
					this) THROWS {
	}

	TcpConnectionListener* setListener(TcpConnectionListener* listener) {
		TcpConnectionListener* prevListener = _listener;
		_listener = listener;
		return prevListener;
	}

	void connect(IPv4::SockAddr addr) THROWS {
		_remoteAddr = addr;
		_conn->connect(_proxyAddr);
	}
	void accept(int server) THROWS {
		THROW(new Utils::Exception("Unsupported"));
	}

	IPv4::SockAddr getLocalAddr() const {
		return _conn->getLocalAddr();
	}
	IPv4::SockAddr getRemoteAddr() const {
		return _remoteAddr;
	}

	void close() {
		_close();
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
