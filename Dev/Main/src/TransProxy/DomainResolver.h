#include "Base/Utils.h"
#include "Base/Debug.h"
#include "HTTP.h"

#pragma once

namespace TransProxy {

class DomainResolver: public HttpService {
	struct ResolvItem;

public:
	class Rules {
		friend class DomainResolver;
		Rules* _next;
	public:
		virtual ~Rules() {
		}
		virtual bool acceptProxy(uint32_t client,
				const char* hostname) const = 0;
		virtual bool denyProxy(uint32_t client, const char* hostname) const = 0;
	};

private:
	struct ResolvItemByName: Utils::MapItemPtr<Utils::String, ResolvItem> {
		ResolvItemByName(ResolvItem* p) :
				Utils::MapItemPtr<Utils::String, ResolvItem>(p) {
		}
		Utils::String getKey() const;
		Utils::String getKeyString() const;
	};

	struct ResolvItemByIP: Utils::MapItemPtr<uint32_t, ResolvItem> {
		ResolvItemByIP(ResolvItem* p) :
				Utils::MapItemPtr<uint32_t, ResolvItem>(p) {
		}
		uint32_t getKey() const;
		Utils::String getKeyString() const;
	};

	struct ResolvItem {
		ResolvItemByName nameItem;
		ResolvItemByIP ipItem;
		Utils::String name;
		uint32_t ip;
		ResolvItem(const char* name, uint32_t ip) :
				nameItem(this), ipItem(this), name(name), ip(ip) {
		}
	};

	uint32_t _ip;
	Rules* _rules;
	Utils::String _cacheFile;

	Utils::Map<Utils::String, ResolvItemByName> _nameToIp;
	Utils::Map<uint32_t, ResolvItemByIP> _ipToName;

	ResolvItem* _add(const char* hostname) THROWS;

public:
	DomainResolver(const char* ipBase, const char* cacheFile);
	virtual ~DomainResolver() {
		Utils::Log::e("~DomainResolver");
	}

	void addRules(Rules* rules) {
		rules->_next = _rules;
		_rules = rules;
	}

	void add(const char* hostname) THROWS;
	uint32_t dns(uint32_t client, const char* hostname) THROWS;
	const char* ddns(uint32_t ip) THROWS;

	// HttpService
	bool onHttpRequest(Net::HttpRequest& request, Net::HttpResponse& response)
			THROWS;
	bool onHttpRequest(Net::HttpRequest& request, Utils::JSONObject& response)
			THROWS;
};

}
