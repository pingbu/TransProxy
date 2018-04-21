#include "Base/Debug.h"
#include "Base/Log.h"
#include "Base/Map.h"
#include "DomainResolver.h"

#pragma once

namespace TransProxy {

struct DomainNode: Utils::MapItem<Utils::String> {
	Utils::String domain;
	Utils::Map<Utils::String, DomainNode> childs;
	DomainNode(const Utils::String& domain) :
			domain(domain), childs(NULL) {
	}
	Utils::String getKey() const {
		return domain;
	}
	Utils::String getKeyString() const {
		return domain;
	}
};

class DomainRules: public DomainResolver::Rules {
	Utils::Map<Utils::String, DomainNode> _proxyDomains, _directDomains;

	static DomainNode* _add(Utils::Map<Utils::String, DomainNode>* domains,
			const char* domain, size_t length);
	static Utils::Map<Utils::String, DomainNode>* _add(
			Utils::Map<Utils::String, DomainNode>* domains, const char* domain);

	static bool _inList(const Utils::Map<Utils::String, DomainNode>* domains,
			const char* hostname);

public:
	DomainRules(const char* ruleFile) THROWS;
	~DomainRules() {
		Utils::Log::e("~DomainRules");
	}

	bool acceptProxy(uint32_t client, const char* hostname) const;
	bool denyProxy(uint32_t client, const char* hostname) const;
};

}
