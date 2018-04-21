#define LOG_TAG "SSLConnection"

#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "Base/Utils.h"
#include "Base/Debug.h"
#include "SSLConnection.h"

namespace Net {

SSL_CTX* SSLConnection::_sslCtx = NULL;

static void __setSockNonBlock(int socket) THROWS {
	int flags = ::fcntl(socket, F_GETFL);
	THROW_IF(flags == -1, new Utils::Exception("F_GETFL failed."));
	flags |= O_NONBLOCK;
	int r = ::fcntl(socket, F_SETFL, flags);
	THROW_IF(r == -1, new Utils::Exception("F_SETFL failed."));
}

SSLConnection::SSLConnection(TcpConnection* conn,
		TcpConnectionListener* listener) :
		_conn(conn), _listener(listener), _ssl(NULL), _ready(false), _baseListener(
				this) THROWS {

	if (_sslCtx == NULL) {
		::SSL_library_init();
		::SSL_load_error_strings();
		const SSL_METHOD* method = ::SSLv23_client_method();
		THROW_IF(method == NULL,
				new Utils::Exception("SSL_client_method failed."));
		_sslCtx = ::SSL_CTX_new(method);
		THROW_IF(_sslCtx == NULL, new Utils::Exception("SSL_CTX_new failed."));
	}

	listener = conn->setListener(&_baseListener);
	if (_listener == NULL)
		_listener = listener;

	::socketpair(AF_UNIX, SOCK_STREAM, 0, _pipe);
	__setSockNonBlock(_pipe[0]);
	__setSockNonBlock(_pipe[1]);
}

void SSLConnection::_doHandshake() THROWS {
	_transferRecv();
	int r = ::SSL_do_handshake(_ssl);
	_transferSend();
	Utils::Log::d("%d <-- SSL_do_handshake", r);
	if (r == 1) {
		_ready = true;
		_listener->onTcpConnected();
	} else {
		int err = ::SSL_get_error(_ssl, r);
		Utils::Log::d("%d <-- SSL_get_error", err);
		if (err == SSL_ERROR_WANT_READ)
			_conn->waitToRecv();
		else if (err == SSL_ERROR_WANT_WRITE)
			_conn->waitToSend();
		else
			THROW(new Utils::Exception("SSL handshake FAILED %d", err));
	}
}

void SSLConnection::_transferRecv() THROWS {
	for (;;) {
		uint8_t buf[16384];
		size_t l = _conn->peek(buf, sizeof(buf));
		if (l == 0)
			break;
		ssize_t r = ::send(_pipe[1], buf, l, 0);
		if (r > 0) {
			Utils::Log::d("_transferRecv %d bytes", r);
			_conn->recv(buf, r);
		} else {
			THROW_IF(r < 0 && errno != EAGAIN,
					new Utils::Exception("send FAILED, errno=%d", errno));
			break;
		}
	}
}

void SSLConnection::_transferSend() THROWS {
	for (;;) {
		uint8_t buf[16384];
		ssize_t r = ::recv(_pipe[1], buf, sizeof(buf), MSG_PEEK | MSG_DONTWAIT);
		if (r > 0) {
			size_t l = _conn->send(buf, r);
			if (l == 0)
				break;
			Utils::Log::d("_transferSend %u bytes", l);
			::recv(_pipe[1], buf, l, 0);
		} else {
			THROW_IF(r < 0 && errno != EAGAIN,
					new Utils::Exception("recv FAILED, errno=%d", errno));
			break;
		}
	}
}

void SSLConnection::_::onTcpConnected() THROWS {
	Utils::Log::d("onTcpConnected");
	_this->_ssl = ::SSL_new(_sslCtx);
	::SSL_set_fd(_this->_ssl, _this->_pipe[0]);
	::SSL_set_connect_state(_this->_ssl);
	_this->_doHandshake();
}

void SSLConnection::_::onTcpDisconnected() THROWS {
	Utils::Log::d("onTcpDisconnected");
	_this->_listener->onTcpDisconnected();
}

void SSLConnection::_::onTcpToRecv() THROWS {
	Utils::Log::d("onTcpCanRecv");
	if (!_this->_ready)
		_this->_doHandshake();
	else
		_this->_listener->onTcpToRecv();
}

void SSLConnection::_::onTcpToSend() THROWS {
	Utils::Log::d("onTcpCanSend");
	if (!_this->_ready)
		_this->_doHandshake();
	else
		_this->_listener->onTcpToSend();

}

void SSLConnection::_::onTcpError(Utils::Exception* e) THROWS {
	_this->_listener->onTcpError(e);
}

void SSLConnection::connect(IPv4::SockAddr addr) THROWS {
	_conn->connect(addr);
}

void SSLConnection::_close() {
	if (_ssl) {
		_conn->close();
		_conn = NULL;
		::SSL_free(_ssl);
		_ssl = NULL;
	}
}

size_t SSLConnection::peek(void* data, size_t bytes) THROWS {
	ASSERT(_ssl);
	_transferRecv();
	ssize_t r = ::SSL_peek(_ssl, data, bytes);
	if (r < 0 && errno == EAGAIN)
		r = 0;
	THROW_IF(r < 0, new Utils::Exception("peek FAILED, errno=%d", errno));
	return r;
}

size_t SSLConnection::recv(void* data, size_t bytes) THROWS {
	ASSERT(_ssl);
	_transferRecv();
	ssize_t r = ::SSL_read(_ssl, data, bytes);
	if (r < 0 && errno == EAGAIN)
		r = 0;
	THROW_IF(r < 0, new Utils::Exception("receive FAILED, errno=%d",errno));
	if (r > 0) {
		Utils::Log::v("%s ----> %d bytes.",
				_conn->getRemoteAddr().toString().sz(), r);
		Utils::Log::dump(data, r);
	}
	return r;
}

size_t SSLConnection::send(const void* data, size_t bytes) THROWS {
	ASSERT(_ssl);
	ssize_t r = ::SSL_write(_ssl, data, bytes);
	if (r < 0 && errno == EAGAIN)
		r = 0;
	THROW_IF(r < 0, new Utils::Exception("send FAILED, errno=%d", errno));
	if (r > 0) {
		_transferSend();
		Utils::Log::v("%s <---- %d bytes...",
				_conn->getRemoteAddr().toString().sz(), r);
		Utils::Log::dump(data, r);
	}
	return r;
}

}
