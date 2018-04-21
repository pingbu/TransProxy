#define LOG_TAG "ProxySocksConnection"

#include "Base/Utils.h"
#include "ProxySocksConnection.h"

namespace Net {

void ProxySocksConnection::_::onTcpConnected() THROWS {
	Utils::Log::i("Authenticating...");
	size_t r = _this->_conn->send("\5\1", 3);
	ASSERT(r == 3);
	_this->_conn->waitToRecv();
}

void ProxySocksConnection::_::onTcpToRecv() THROWS {
	if (_this->_ready) {
		_this->_listener->onTcpToRecv();
		return;
	}

	// Authenticate response
	TRY{
		uint8_t buf[2];
		size_t bytes = _this->_conn->peek(buf, sizeof(buf));
		if (bytes >= 2 && buf[0] == 5 && buf[1] == 0) {
			_this->_conn->recv(buf, sizeof(buf));
			Utils::Log::i("Authenticated PASS.");
			_this->_ready = true;
			_this->_listener->onTcpConnected();
		} else {
			THROW(new Utils::Exception("SOCKS proxy authenticate FAILED"));
		}
	}CATCH(e){
		_this->_listener->onTcpError(e);
	}
}

}
