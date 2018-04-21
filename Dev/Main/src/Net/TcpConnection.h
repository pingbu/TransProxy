#include <stddef.h>
#include "Base/Debug.h"
#include "IPv4.h"

#pragma once

namespace Net {

struct TcpConnectionListener {
	virtual ~TcpConnectionListener() {
	}
	virtual void onTcpConnected() THROWS = 0;
	virtual void onTcpDisconnected() THROWS = 0;
	virtual void onTcpToRecv() THROWS = 0;
	virtual void onTcpToSend() THROWS = 0;
	virtual void onTcpError(Utils::Exception* e) THROWS = 0;
};

struct TcpConnection {
	virtual TcpConnectionListener* setListener(
			TcpConnectionListener* listener) = 0;
	virtual void connect(IPv4::SockAddr addr) THROWS = 0; // Client Only
	virtual IPv4::SockAddr getLocalAddr() const = 0;
	virtual IPv4::SockAddr getRemoteAddr() const = 0;
	virtual void close() THROWS = 0;
	virtual void waitToRecv() = 0;
	virtual void waitToSend() = 0;
	virtual size_t peek(void* data, size_t bytes) THROWS = 0;
	virtual size_t recv(void* data, size_t bytes) THROWS = 0;
	virtual size_t send(const void* data, size_t bytes) THROWS = 0;

protected:
	virtual ~TcpConnection() {
	}
};

}
