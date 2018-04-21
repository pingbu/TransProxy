#include <netinet/in.h>
#include "Base/Utils.h"
#include "Base/Looper.h"
#include "TcpConnection.h"

#pragma once

namespace Net {

class SocketConnection: public TcpConnection {
	friend class SSLConnection;

	int _socket, _selector;
	bool _connected;
	TcpConnectionListener* _listener;
	Net::IPv4::SockAddr _localAddr, _remoteAddr;

	struct _: Utils::FDListener {
		SocketConnection* _this;
		_(SocketConnection* thiz) :
				_this(thiz) {
		}
		void onFDToRead() THROWS {
			_this->_listener->onTcpToRecv();
		}
		void onFDToWrite() THROWS;
		void onFDClosed() THROWS;
		void onFDError(Utils::Exception* e) THROWS {
			_this->_listener->onTcpError(e);
		}

	} _fdListener;

	void _close();

protected:
	~SocketConnection() {
		_close();
	}

public:
	SocketConnection(TcpConnectionListener* listener = NULL) :
			_socket(-1), _selector(-1), _connected(false), _listener(listener), _fdListener(
					this) THROWS {
	}

	TcpConnectionListener* setListener(TcpConnectionListener* listener) {
		TcpConnectionListener* prevListener = _listener;
		_listener = listener;
		return prevListener;
	}

	void connect(IPv4::SockAddr addr) THROWS; // Client Only
	void accept(int server) THROWS; // Server Only

	Net::IPv4::SockAddr getLocalAddr() const {
		return _localAddr;
	}
	Net::IPv4::SockAddr getRemoteAddr() const {
		return _remoteAddr;
	}

	void close() {
		_close();
		delete this;
	}

	void waitToRecv() {
		Utils::Looper::myLooper()->waitToRead(_selector);
	}
	void waitToSend() {
		Utils::Looper::myLooper()->waitToWrite(_selector);
	}

	size_t peek(void* data, size_t bytes) THROWS;
	size_t recv(void* data, size_t bytes) THROWS;
	size_t send(const void* data, size_t bytes) THROWS;
};

}
