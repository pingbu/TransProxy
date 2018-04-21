#include <stdint.h>
#include "Base/Debug.h"
#include "Base/Utils.h"
#include "ProxyAuth.h"

#pragma once

namespace TransProxy {

class ProxyAuthSock5: public ProxyAuth {
	ProxyAuthListener* _listener;
	Net::PacketBuffer<Net::Packet> _req;
	int _state;
	char _response[1024];
	size_t _responseBytes;
public:
	ProxyAuthSock5(const char* hostname, uint16_t port,
			ProxyAuthListener* listener) :
			_listener(listener), _req(::strlen(hostname) + 7), _state(1), _responseBytes(
					0) {
		uint8_t l = ::strlen(hostname);
		_req.write(0, "\5\1\0\3", 4);
		_req[4] = l;
		_req.write(5, hostname, l);
		_req.write16(l + 5, port);
	}
	void sendAuthRequest() THROWS {
		if (_state == 1)
			_listener->sendAuthRequest("\5\1", 3,
					sizeof(_response) - _responseBytes);
		else if (_state == 2)
			_listener->sendAuthRequest(_req.ptr(), _req.size(),
					sizeof(_response) - _responseBytes);
	}
	int onAuthResponse(const void* data, size_t bytes) THROWS {
		if (_responseBytes + bytes > sizeof(_response)) {
			Utils::Log::e("HTTP proxy connection response overflow");
			return 0;
		}
		::memcpy(_response + _responseBytes, data, bytes);
		_responseBytes += bytes;
		if (_state == 1) {
			if (_responseBytes >= 2) {
				if (_responseBytes == 2 && _response[0] == 5
						&& _response[1] == 0) {
					_state = 2;
					_responseBytes = 0;
					_listener->commitAuthRequest(3);
					sendAuthRequest();
					return -1;
				}
				return 0;
			}
		} else if (_state == 2) {
			if (_responseBytes >= 7) {
				if (_response[0] == 5 && _response[1] == 0) {
					size_t l = 0;
					if (_response[3] == 1)
						l = 10;
					else if (_response[3] == 3)
						l = 7 + _response[4];
					if (_responseBytes < l)
						return -1;
					_listener->commitAuthRequest(_req.size());
					return 1;
				}
				return 0;
			}
		}
		return -1;
	}

	struct Builder: ProxyAuthBuilder {
		ProxyAuth* createInstance(const char* hostname, uint16_t port,
				ProxyAuthListener* listener) THROWS {
			return new ProxyAuthSock5(hostname, port, listener);
		}
	};
};

}
