#include <stdint.h>
#include "Base/Utils.h"
#include "TcpConnection.h"

#pragma once

namespace Net {

struct TcpClientFactory {
	virtual ~TcpClientFactory() {
	}
	virtual TcpConnection* tcp_open(TcpConnectionListener* listener = NULL)
			THROWS = 0;
};

}
