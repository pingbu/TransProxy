#include "Base/Utils.h"
#include "Net/IPv4.h"
#include "TcpConnection.h"

#pragma once

namespace Net {

struct TcpServerListener {
	virtual ~TcpServerListener() {
	}
	virtual TcpConnectionListener* onTcpServerConnected(TcpConnection* conn)
			THROWS = 0;
	virtual void onTcpServerError(Utils::Exception* e) THROWS = 0;
};

struct TcpServer {
	virtual ~TcpServer() {
	}
	virtual uint16_t getPort() const = 0;
};

struct TcpServerFactory {
	virtual ~TcpServerFactory() {
	}
	TcpServer* tcp_listen(TcpServerListener* listener) THROWS {
		return tcp_listen(0, listener);
	}
	virtual TcpServer* tcp_listen(uint16_t port, TcpServerListener* listener)
			THROWS = 0;
};

}
