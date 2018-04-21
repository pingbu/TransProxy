#define LOG_TAG "TcpServerDirect"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "Base/Utils.h"
#include "SocketConnection.h"
#include "TcpServerDirect.h"

namespace Net {

TcpServerDirect::TcpServerDirect(TcpServerListener* listener, uint16_t port) :
		_listener(listener), _port(port), _fdListener(this) THROWS {
	Utils::Log::i("TCP Server Direct initializing...");

	_server = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	THROW_IF(_server < 0,
			new Utils::Exception("Create TCP server socket failed."));

	TRY{
		struct sockaddr_in addr = {0};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(port);
		int r = ::bind(_server, (const struct sockaddr*) &addr, sizeof(addr));
		THROW_IF (r != 0,
				new Utils::Exception("Bind TCP server socket failed."));

		if (port == 0) {
			size_t n = sizeof(addr);
			r = ::getsockname(_server, (struct sockaddr*) &addr, &n);
			THROW_IF (r != 0,
					new Utils::Exception("Listen failed, errno=%d.", errno));
			_port = ntohs(addr.sin_port);
		}

		r = ::listen(_server, SOMAXCONN);
		THROW_IF (r != 0,
				new Utils::Exception("Listen failed."));

		_selector = Utils::Looper::myLooper()->attachFD(_server, &_fdListener);
		Utils::Log::i("#%d listening at port %u...", _selector, _port);
		Utils::Looper::myLooper()->waitToRead(_selector);
	}CATCH (e){
		::close(_server);
		THROW(e);
	}
}

TcpServerDirect::~TcpServerDirect() {
	::close(_server);
}

void TcpServerDirect::_::onFDToRead() THROWS {
	SocketConnection* conn = new SocketConnection();
	conn->setListener(_this->_listener->onTcpServerConnected(conn));
	Utils::Looper::myLooper()->waitToRead(_this->_selector);
}

}
