#define LOG_TAG "DomainRules"

#include "Base/Debug.h"
#include "Base/Utils.h"
#include "DomainRules.h"

namespace TransProxy {

DomainNode* DomainRules::_add(Utils::Map<Utils::String, DomainNode>* domains,
		const char* domain, size_t length) {
	Utils::String s(domain, length);
	DomainNode* node = domains->get(s);
	if (node == NULL) {
		node = new DomainNode(s);
		domains->add(node);
	}
	return node;
}

Utils::Map<Utils::String, DomainNode>* DomainRules::_add(
		Utils::Map<Utils::String, DomainNode>* domains, const char* domain) {
	size_t l = ::strlen(domain);
	while (l && (uint8_t) domain[l - 1] < (uint8_t) ' ')
		--l;
	for (size_t begin = l, end = l;; --begin) {
		if (begin == 0 || domain[begin - 1] == '.') {
			if (begin == end)
				return NULL;
			if (begin == 0)
				break;
			end = begin - 1;
		}
	}
	for (size_t begin = l, end = l;; --begin) {
		if (begin == 0 || domain[begin - 1] == '.') {
			DomainNode* node = _add(domains, domain + begin, end - begin);
			domains = &node->childs;
			if (begin == 0)
				break;
			end = begin - 1;
		}
	}
	return domains;
}

DomainRules::DomainRules(const char* ruleFile) THROWS {
	Utils::Log::i("DomainRules initializing...");
	int count = 0;
	FILE* fp = ::fopen(ruleFile, "rt");
	if (fp) {
		char l[2048];
		while (::fgets(l, sizeof(l), fp)) {
			//Utils::Log::v("%s", l);
			const char* p = l;
			while (*p && (uint8_t) *p <= (uint8_t) ' ')
				++p;
			if (!*p || p[0] == '!')
				continue;
			Utils::Map<Utils::String, DomainNode>* domains = &_proxyDomains;
			if (p[0] == '@' && p[1] == '@') {
				domains = &_directDomains;
				p += 2;
			}
			const char* q = ::strstr(p, "://");
			if (q)
				p = q + 3;
			size_t m;
			q = ::strchr(p, '/');
			if (q)
				m = q - p;
			else
				m = ::strlen(p);
			if (m == 0 || p[0] == '/')
				continue;
			q = ::strchr(p, '.');
			if (!q || q >= p + m)
				continue;
			if (p[0] == '.') {
				domains = _add(domains, p + 1);
				if (domains) {
					_add(domains, "*", 1);
					++count;
				}
			} else if (p[0] == '|' && p[1] == '|') {
				domains = _add(domains, p + 2);
				if (domains) {
					_add(domains, "*", 1);
					++count;
				}
			} else {
				domains = _add(domains, p);
				if (domains) {
					_add(domains, ".", 1);
					++count;
				}
			}
		}
		::fclose(fp);
	}
	Utils::Log::i("%d domains loaded from rules file", count);
}

bool DomainRules::_inList(const Utils::Map<Utils::String, DomainNode>* domains,
		const char* hostname) {
	size_t l = ::strlen(hostname);
	for (size_t begin = l, end = l;; --begin) {
		if (begin == 0 || hostname[begin - 1] == '.') {
			DomainNode* node = domains->get("*");
			if (node != NULL)
				return true;
			node = domains->get(Utils::String(hostname + begin, end - begin));
			if (node == NULL)
				return false;
			domains = &node->childs;
			if (begin == 0)
				break;
			end = begin - 1;
		}
	}
	return domains->get(".") != NULL || domains->get("*") != NULL;
}

bool DomainRules::acceptProxy(uint32_t client, const char* hostname) const {
	bool r = _inList(&_proxyDomains, hostname);
	if (r)
		Utils::Log::d("Host '%s' accept proxy.", hostname);
	return r;
}

bool DomainRules::denyProxy(uint32_t client, const char* hostname) const {
	bool r = _inList(&_directDomains, hostname);
	if (r)
		Utils::Log::d("Host '%s' deny proxy.", hostname);
	return r;
}

}
