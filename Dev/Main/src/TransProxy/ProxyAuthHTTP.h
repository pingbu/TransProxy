#include <stdint.h>
#include "Base/Debug.h"
#include "Base/Utils.h"
#include "ProxyAuth.h"

#pragma once

namespace TransProxy {

class ProxyAuthHTTP: public ProxyAuth {
	ProxyAuthListener* _listener;
	Utils::String _req;
	char _response[1024];
	size_t _responseBytes;
public:
	ProxyAuthHTTP(const char* hostname, uint16_t port,
			ProxyAuthListener* listener) :
			_listener(listener), _responseBytes(0) {
		_req = Utils::String::format("CONNECT %s:%u HTTP/1.1\r\n"
				"Host: %s:%u\r\n"
				"Proxy-Connection: Keep-Alive\r\n"
				"Content-Length: 0\r\n"
				"\r\n", hostname, port, hostname, port);
	}
	void sendAuthRequest() THROWS {
		_listener->sendAuthRequest(_req.sz(), _req.length(),
				sizeof(_response) - 1 - _responseBytes);
	}
	int onAuthResponse(const void* data, size_t bytes) THROWS {
		if (_responseBytes + bytes >= sizeof(_response)) {
			Utils::Log::e("HTTP proxy connection response overflow");
			return 0;
		}
		::memcpy(_response + _responseBytes, data, bytes);
		_responseBytes += bytes;
		_response[_responseBytes] = '\0';
		char* p = ::strstr(_response, "\r\n\r\n");
		if (p) {
			if (::strncmp(_response, "HTTP/1.1 200 ", 13) != 0) {
				Utils::Log::e("FAILED to connect on HTTP proxy");
				return 0;
			}
			_listener->commitAuthRequest(_req.length());
			return 1;
		}
		return -1;
	}

	struct Builder: ProxyAuthBuilder {
		ProxyAuth* createInstance(const char* hostname, uint16_t port,
				ProxyAuthListener* listener) THROWS {
			return new ProxyAuthHTTP(hostname, port, listener);
		}
	};
};

}
