#include "Base/Debug.h"

#pragma once

namespace Net {

struct DnsQueryListener {
	virtual ~DnsQueryListener() {
	}
	virtual void onDnsQueryResult(uint32_t ip) THROWS = 0;
	virtual void onDnsQueryError(Utils::Exception* e) THROWS = 0;
};

class DnsQuery {
protected:
	virtual ~DnsQuery() {
	}
public:
	virtual void cancel() THROWS = 0;
};

struct DnsClient {
	virtual ~DnsClient() {
	}
	virtual DnsQuery* query(const char* hostname, DnsQueryListener* listener)
			THROWS = 0;
};

}
