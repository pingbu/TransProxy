#define LOG_TAG "SocketConnection"

#include <errno.h>
#include "Base/Utils.h"
#include "Base/Debug.h"
#include "SocketConnection.h"

namespace Net {

static IPv4::SockAddr __getLocalAddr(int socket) THROWS {
	struct sockaddr_in addr;
	size_t n = sizeof(addr);
	int r = ::getsockname(socket, (struct sockaddr*) &addr, &n);
	THROW_IF(r != 0,
			new Utils::Exception("getsockname failed, errno=%d.", errno));
	return Net::IPv4::SockAddr(ntohl(addr.sin_addr.s_addr),
			ntohs(addr.sin_port));
}

//static IPv4::SockAddr __getRemoteAddr(int socket) THROWS {
//	struct sockaddr_in addr;
//	size_t n = sizeof(addr);
//	int r = ::getpeername(socket, (struct sockaddr*) &addr, &n);
//	THROW_IF(r != 0,
//			new Utils::Exception("getpeername failed, errno=%d.", errno));
//	return Net::IPv4::SockAddr(ntohl(addr.sin_addr.s_addr),
//			ntohs(addr.sin_port));
//}

void SocketConnection::connect(IPv4::SockAddr addr) THROWS {
	_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	THROW_IF(_socket < 0,
			new Utils::Exception("Create TCP client socket failed."));

	TRY{
		_selector = Utils::Looper::myLooper()->attachFD(_socket, &_fdListener);

		TRY{
			struct sockaddr_in localAddr = {0};
			localAddr.sin_family = AF_INET;
			localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
			localAddr.sin_port = htons(0);
			int r = ::bind(_socket, (const struct sockaddr*) &localAddr, sizeof(localAddr));
			THROW_IF(r != 0, new Utils::Exception("Bind TCP client socket failed."));

			_remoteAddr = addr;
			_localAddr = __getLocalAddr(_socket);

			struct sockaddr_in remoteAddr = {0};
			remoteAddr.sin_family = AF_INET;
			remoteAddr.sin_addr.s_addr = htonl(addr.ip);
			remoteAddr.sin_port = htons(addr.port);
			r = ::connect(_socket, (const struct sockaddr*) &remoteAddr,
					sizeof(remoteAddr));
			THROW_IF(r != 0 && errno != EINPROGRESS,
					new Utils::Exception("Connect FAILED."));

			Utils::Log::i("#%d connecting TCP from port %d", _selector, _localAddr.port);
			Utils::Looper::myLooper()->waitToWrite(_selector);
		}CATCH (e){
			Utils::Looper::myLooper()->detachFD(_selector);
			THROW(e);
		}
	}CATCH (e){
		::close(_socket);
		THROW(e);
	}
}

void SocketConnection::accept(int server) THROWS {
	struct sockaddr_in addr = { 0 };
	socklen_t n = sizeof(addr);
	_socket = ::accept(server, (struct sockaddr*) &addr, &n);
	THROW_IF(_socket < 0,
			new Utils::Exception("Accept FAILED, errno=%d.", errno));

	TRY{
		_selector = Utils::Looper::myLooper()->attachFD(_socket, &_fdListener);

		TRY{
			_remoteAddr = Net::IPv4::SockAddr(ntohl(addr.sin_addr.s_addr),
					ntohs(addr.sin_port));
			_localAddr = __getLocalAddr(_socket);

			Utils::Log::i("#%d accepted connection from %s", _selector,
					_remoteAddr.toString().sz());
		}CATCH (e){
			Utils::Looper::myLooper()->detachFD(_selector);
			THROW(e);
		}
	}CATCH (e){
		::close(_socket);
		THROW(e);
	}
}

void SocketConnection::_::onFDToWrite() THROWS {
	if (_this->_connected) {
		_this->_listener->onTcpToSend();
	} else {
		int error = 0;
		socklen_t sz = sizeof(error);
		int r = ::getsockopt(_this->_socket, SOL_SOCKET, SO_ERROR, &error, &sz);
		if (r == 0 && error == 0) {
			_this->_connected = true;
			struct sockaddr_in addr;
			size_t n = sizeof(addr);
			int r = ::getpeername(_this->_socket, (struct sockaddr*) &addr, &n);
			THROW_IF(r != 0,
					new Utils::Exception("getpeername failed, errno=%d.", errno));
			_this->_remoteAddr = Net::IPv4::SockAddr(
					ntohl(addr.sin_addr.s_addr), ntohs(addr.sin_port));

			Utils::Log::i("#%d connected to %s", _this->_selector,
					_this->_remoteAddr.toString().sz());
			_this->_listener->onTcpConnected();
		}
	}
}

void SocketConnection::_::onFDClosed() THROWS {
	Utils::Log::i("#%d disconnected.", _this->_selector);
	_this->_listener->onTcpDisconnected();
}

void SocketConnection::_close() {
	if (_socket >= 0) {
		Utils::Looper::myLooper()->detachFD(_selector);
		::close(_socket);
		_socket = -1;
		Utils::Log::i("#%d closed.", _selector);
	}
}

size_t SocketConnection::peek(void* data, size_t bytes) THROWS {
	ASSERT(_socket >= 0);
	ssize_t r = ::recv(_socket, data, bytes, MSG_PEEK | MSG_DONTWAIT);
	if (r < 0 && errno == EAGAIN)
		r = 0;
	THROW_IF(r < 0,
			new Utils::Exception("#%d peek FAILED, errno=%u", _selector, errno));
	return r;
}

size_t SocketConnection::recv(void* data, size_t bytes) THROWS {
	ASSERT(_socket >= 0);
	ssize_t r = ::recv(_socket, data, bytes, 0);
	if (r < 0 && errno == EAGAIN)
		r = 0;
	THROW_IF(r < 0,
			new Utils::Exception("#%d receive FAILED, errno=%u", _selector, errno));
	Utils::Log::v("#%d %s ----> %u bytes.", _selector,
			_remoteAddr.toString().sz(), r);
	Utils::Log::dump(data, r);
	return r;
}

size_t SocketConnection::send(const void* data, size_t bytes) THROWS {
	ASSERT(_socket >= 0);
	ssize_t r = ::send(_socket, data, bytes, 0);
	if (r < 0 && errno == EAGAIN)
		r = 0;
	THROW_IF(r < 0,
			new Utils::Exception("#%d send FAILED, errno=%u", _selector, errno));
	Utils::Log::v("#%d %s <---- %u bytes...", _selector,
			_remoteAddr.toString().sz(), r);
	Utils::Log::dump(data, r);
	return r;
}

}
