#define LOG_TAG "ProxySocksTcp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "Base/Debug.h"
#include "Base/Utils.h"
#include "ProxySocksTcp.h"

namespace Net {

void ProxySocksTcp::_::onTcpConnected() THROWS {
//	if (_this->_ip) {
	Utils::Log::i("Connecting to %s...", _this->_remoteAddr.toString().sz());
	PacketFixedBuffer<Packet, 10> req;
	req[0] = 5;
	req[1] = 1;
	req[2] = 0;
	req[3] = 1;
	req.write32(4, _this->_remoteAddr.ip);
	req.write16(8, _this->_remoteAddr.port);
	size_t r = _this->_conn->send(req.ptr(), req.size());
	ASSERT(r == req.size());
//	} else {
//		Utils::Log::i("Connecting to %s:%u...", _this->_hostname.sz(),
//				_this->_port);
//		uint8_t l = _this->_hostname.length();
//		PacketBuffer<Packet> req(l + 7);
//		req[0] = 5;
//		req[1] = 1;
//		req[2] = 0;
//		req[3] = 3;
//		req[4] = l;
//		req.write(5, _this->_hostname.sz(), l);
//		req.write16(l + 5, _this->_port);
//		size_t r = _this->_conn->send(req.ptr(), req.size());
//		ASSERT(r == req.size());
//	}
	_this->_conn->waitToRecv();
}

void ProxySocksTcp::_::onTcpToRecv() THROWS {
	if (_this->_ready) {
		_this->_listener->onTcpToRecv();
		return;
	}

	// Connect response
	Utils::Exception* e = NULL;
	for (;;) {
		char buf[128];
		size_t bytes = _this->_conn->peek(buf, sizeof(buf));
		if (bytes > 6 && buf[0] == 5) {
			if (buf[1] != 0) {
				e = new Utils::Exception("Connect FAILED #%d", buf[1]);
				e->setTrace(__FUNCTION__, __FILE__, __LINE__);
				break;
			}
			size_t l = 0;
			if (buf[3] == 1)
				l = 10;
			else if (buf[3] == 3)
				l = 7 + buf[4];
			if (bytes >= l) {
				_this->_conn->recv(buf, l);
				//if (_this->_ip)
				Utils::Log::i("Connected to %s",
						_this->_remoteAddr.toString().sz());
//				else
//					Utils::Log::i("Connected to %s:%u", _this->_hostname.sz(),
//							_this->_port);
				_this->_ready = true;
				_this->_listener->onTcpConnected();
				return;
			}
		}
		break;
	}
	if (e == NULL) {
		e = new Utils::Exception("Connect FAILED");
		e->setTrace(__FUNCTION__, __FILE__, __LINE__);
	}
	_this->_listener->onTcpError(e);
}

}
