#include "Base/Debug.h"
#include "TcpClient.h"

#pragma once

namespace Net {

TcpClientFactory* newProxy(TcpClientFactory* factory, const char* url) THROWS;

}
