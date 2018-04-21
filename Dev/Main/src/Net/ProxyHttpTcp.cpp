#define LOG_TAG "ProxyHttpTcp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "Base/Debug.h"
#include "Base/Utils.h"
#include "ProxyHttpTcp.h"

namespace Net {

void ProxyHttpTcp::_::onTcpConnected() THROWS {
	Utils::String req;
//	if (_this->_ip) {
	const char* ip = IPv4::ntoa(_this->_remoteAddr.ip);
	Utils::Log::i("Connecting to %s:%u...", ip, _this->_remoteAddr.port);
	req = Utils::String::format("CONNECT %s:%u HTTP/1.1\r\n"
			"Host: %s:%u\r\n"
			"Proxy-Connection: Keep-Alive\r\n"
			"Content-Length: 0\r\n"
			"\r\n", ip, _this->_remoteAddr.port, ip, _this->_remoteAddr.port);
//	} else {
//		Utils::Log::i("Connecting to %s:%u...", _this->_hostname.sz(),
//				_this->_port);
//		req = Utils::String::format("CONNECT %s:%u HTTP/1.1\r\n"
//				"Host: %s:%u\r\n"
//				"Proxy-Connection: Keep-Alive\r\n"
//				"Content-Length: 0\r\n"
//				"\r\n", _this->_hostname.sz(), _this->_port,
//				_this->_hostname.sz(), _this->_port);
//	}
	size_t r = _this->_conn->send(req.sz(), req.length());
	ASSERT(r == req.length());
	_this->_conn->waitToRecv();
}

void ProxyHttpTcp::_::onTcpToRecv() THROWS {
	if (_this->_ready) {
		_this->_listener->onTcpToRecv();
		return;
	}

	// Connect response
	Utils::Exception* e = NULL;
	char buf[1024];
	size_t bytes = _this->_conn->peek(buf, sizeof(buf) - 1);
	if (bytes >= 13) {
		buf[bytes] = '\0';
		char* p = ::strstr(buf, "\r\n\r\n");
		if (!p) {
			_this->_conn->waitToRecv();
			return;
		}
		if (::strncmp(buf, "HTTP/1.1 200 ", 13) == 0) {
			size_t hdrlen = p + 4 - buf;
			_this->_conn->recv(buf, hdrlen);
			_this->_ready = true;
//			if (_this->_ip)
			Utils::Log::i("Connected to %s",
					_this->_remoteAddr.toString().sz());
//			else
//				Utils::Log::i("Connected to %s:%u", _this->_hostname.sz(),
//						_this->_port);
			_this->_listener->onTcpConnected();
			return;
		}
	}
	if (e == NULL) {
		e = new Utils::Exception("Connect FAILED");
		e->setTrace(__FUNCTION__, __FILE__, __LINE__);
	}
	_this->_listener->onTcpError(e);
}

}
