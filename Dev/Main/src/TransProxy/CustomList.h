#include <stdio.h>
#include "Base/Debug.h"
#include "Base/Map.h"
#include "DomainResolver.h"

#pragma once

namespace TransProxy {

class CustomList: public DomainResolver::Rules {
	Utils::Map<Utils::String, Utils::StringSetItem> _proxyList, _directList;

	static void _loadList(Utils::Map<Utils::String, Utils::StringSetItem>& list,
			const char* file) {
		FILE* fp = ::fopen(file, "rt");
		if (fp) {
			char line[256];
			while (::fgets(line, sizeof(line) - 1, fp)) {
				char* p = line;
				while (*p > '\0' && *p < ' ')
					++p;
				if (*p && *p != '#') {
					char* q = p + ::strlen(p) - 1;
					while (q >= p && *q > '\0' && *q < ' ')
						--q;
					q[1] = '\0';
					list.add(new Utils::StringSetItem(p));
				}
			}
			::fclose(fp);
		}
	}

public:
	CustomList(const char* proxyListFile, const char* directListFile) THROWS {
		_loadList(_proxyList, proxyListFile);
		_loadList(_directList, directListFile);
	}
	~CustomList() {
		Utils::Log::e("~CustomList");
	}

	bool acceptProxy(uint32_t client, const char* hostname) const {
		return _proxyList.get(hostname) != NULL;
	}
	bool denyProxy(uint32_t client, const char* hostname) const {
		return _directList.get(hostname) != NULL;
	}
};

}
