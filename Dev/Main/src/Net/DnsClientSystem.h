#include "Base/Debug.h"
#include "DnsClient.h"

#pragma once

namespace Net {

class DnsClientSystem: public DnsClient {
	DnsQuery* query(const char* hostname, DnsQueryListener* listener) THROWS;
};

}
