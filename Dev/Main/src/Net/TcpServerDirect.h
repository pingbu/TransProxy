#include <stdint.h>
#include <netinet/in.h>
#include "Base/Utils.h"
#include "Base/Looper.h"
#include "TcpServer.h"

#pragma once

namespace Net {

class TcpServerDirect: public TcpServer {
	TcpServerListener* _listener;
	int _server, _selector;
	uint16_t _port;

	struct _: Utils::FDListener {
		TcpServerDirect* _this;
		_(TcpServerDirect* thiz) :
				_this(thiz) {
		}
		void onFDToRead() THROWS;
		void onFDToWrite() {
		}
		void onFDClosed() {
		}
		void onFDError(Utils::Exception* e) THROWS {
			THROW(e);
		}
	} _fdListener;

public:
	TcpServerDirect(TcpServerListener* listener, uint16_t port = 0) THROWS;
	~TcpServerDirect();
	uint16_t getPort() const {
		return _port;
	}

	struct Factory: TcpServerFactory {
		TcpServer* tcp_listen(uint16_t port, TcpServerListener* listener)
				THROWS {
			return new TcpServerDirect(listener, port);
		}
	};
};

}
