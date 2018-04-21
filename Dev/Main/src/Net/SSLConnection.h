#include <openssl/ssl.h>
#include "Base/Debug.h"
#include "TcpConnection.h"

#pragma once

namespace Net {

class SSLConnection: public TcpConnection {
	static SSL_CTX* _sslCtx;

	TcpConnection* _conn;
	TcpConnectionListener* _listener;
	SSL* _ssl;
	bool _ready;
	int _pipe[2];

	struct _: TcpConnectionListener {
		SSLConnection* _this;
		_(SSLConnection* thiz) :
				_this(thiz) {
		}
		void onTcpConnected() THROWS;
		void onTcpDisconnected() THROWS;
		void onTcpToRecv() THROWS;
		void onTcpToSend() THROWS;
		void onTcpError(Utils::Exception* e) THROWS;
	} _baseListener;

	void _doHandshake() THROWS;
	void _transferRecv() THROWS;
	void _transferSend() THROWS;
	void _close();

public:
	SSLConnection(TcpConnection* conn, TcpConnectionListener* listener = NULL)
			THROWS;
	virtual ~SSLConnection() {
		_close();
	}

	TcpConnectionListener* setListener(TcpConnectionListener* listener) {
		TcpConnectionListener* prevListener = _listener;
		_listener = listener;
		return prevListener;
	}

	void connect(IPv4::SockAddr addr) THROWS;

	IPv4::SockAddr getLocalAddr() const {
		return _conn->getLocalAddr();
	}
	IPv4::SockAddr getRemoteAddr() const {
		return _conn->getRemoteAddr();
	}

	void close() {
		_close();
		delete this;
	}

	void waitToRecv() {
		_conn->waitToRecv();
	}
	void waitToSend() {
		_conn->waitToSend();
	}

	size_t peek(void* data, size_t bytes) THROWS;
	size_t recv(void* data, size_t bytes) THROWS;
	size_t send(const void* data, size_t bytes) THROWS;
};

}
