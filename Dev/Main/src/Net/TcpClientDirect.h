#include <netinet/in.h>
#include "Base/Utils.h"
#include "Base/Looper.h"
#include "SocketConnection.h"
#include "TcpClient.h"

#pragma once

namespace Net {

struct TcpClientDirect {
	struct Factory: TcpClientFactory {
		SocketConnection* tcp_open(TcpConnectionListener* listener = NULL)
				THROWS {
			return new SocketConnection(listener);
		}
	};
};

}
