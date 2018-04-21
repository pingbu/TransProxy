#define LOG_TAG "DomainResolver"

#include <unistd.h>
#include "Base/Debug.h"
#include "DomainResolver.h"

namespace TransProxy {

Utils::String DomainResolver::ResolvItemByName::getKey() const {
	return (*this)->name;
}

Utils::String DomainResolver::ResolvItemByName::getKeyString() const {
	return Utils::String::format("%s %s", (*this)->name.sz(),
			Net::IPv4::ntoa((*this)->ip));
}

uint32_t DomainResolver::ResolvItemByIP::getKey() const {
	return (*this)->ip;
}

Utils::String DomainResolver::ResolvItemByIP::getKeyString() const {
	return Utils::String::format("%s %s", Net::IPv4::ntoa((*this)->ip),
			(*this)->name.sz());
}

DomainResolver::DomainResolver(const char* ipBase, const char* workDir) :
		_ip(Net::IPv4::aton(ipBase)), _rules(NULL), _nameToIp("name->ip"), _ipToName(
				"ip->name") THROWS {
	Utils::Log::i("DomainResolver initializing...");

	_cacheFile = workDir;
	_cacheFile += "/dns.cache";

	FILE* fp = ::fopen(_cacheFile, "rb");
	if (fp) {
		size_t size, n = 0;
		if (::fread(&size, 1, sizeof(size), fp) != sizeof(size))
			goto ReadErr;

		while ((size_t) ::ftell(fp) < size) {
			size_t l;
			if (::fread(&l, 1, sizeof(l), fp) != sizeof(l))
				goto ReadErr;

			char hostname[l + 1];
			if (::fread(hostname, 1, l, fp) != l)
				goto ReadErr;
			hostname[l] = '\0';

			uint32_t ip;
			if (::fread(&ip, 1, sizeof(ip), fp) != sizeof(ip))
				goto ReadErr;
			if (ip != _ip)
				goto ReadErr;

			ResolvItem* host = new ResolvItem(hostname, _ip++);
			_nameToIp.add(&host->nameItem);
			_ipToName.add(&host->ipItem);
			++n;
		}
		::fclose(fp);

		Utils::Log::i("%u domains loaded from cache", n);
	}
	return;

	ReadErr: ;
	::fclose(fp);
	::unlink(_cacheFile);
	_ip = Net::IPv4::aton(ipBase);
	_nameToIp.clear();
	_ipToName.clear();
	Utils::Log::e("FAILED to load domains from cache, cache cleared");
}

DomainResolver::ResolvItem* DomainResolver::_add(const char* hostname) THROWS {
	ResolvItem* host = new ResolvItem(hostname, _ip++);
	_nameToIp.add(&host->nameItem);
	_ipToName.add(&host->ipItem);

	size_t size;
	FILE* fp = ::fopen(_cacheFile, "rb+");
	if (fp) {
		if (::fread(&size, 1, sizeof(size), fp) < sizeof(size)) {
			::fclose(fp);
			fp = NULL;
			Utils::Log::w("Invalid cache file header, truncated");
		} else {
			::fseek(fp, size, SEEK_SET);
			if ((size_t) ::ftell(fp) != size) {
				::fclose(fp);
				fp = NULL;
				Utils::Log::w("Invalid cache file body, truncated");
			}
		}
	}
	if (!fp) {
		size = sizeof(size);
		fp = ::fopen(_cacheFile, "wb+");
		::fwrite(&size, 1, sizeof(size), fp);
	}
	size_t l = ::strlen(hostname);
	::fwrite(&l, 1, sizeof(l), fp);
	::fwrite(hostname, 1, l, fp);
	::fwrite(&host->ip, 1, sizeof(host->ip), fp);
	size = ::ftell(fp);
	::fflush(fp);
	::fseek(fp, 0, SEEK_SET);
	::fwrite(&size, 1, sizeof(size), fp);
	::fclose(fp);

	Utils::Log::d("%s <-- dns '%s'", Net::IPv4::ntoa(host->ip), hostname);
	return host;
}

void DomainResolver::add(const char* hostname) THROWS {
	if (!_nameToIp.get(hostname))
		_add(hostname);
}

uint32_t DomainResolver::dns(uint32_t client, const char* hostname) THROWS {
	ResolvItem* host = NULL;
	bool deny = false;
	for (Rules* rules = _rules; rules; rules = rules->_next)
		if (rules->denyProxy(client, hostname)) {
			deny = true;
			break;
		}
	if (!deny)
		for (Rules* rules = _rules; rules; rules = rules->_next)
			if (rules->acceptProxy(client, hostname)) {
				host = *_nameToIp.get(hostname);
				if (host == NULL)
					host = _add(hostname);
				break;
			}
	Utils::Log::d("%s <-- dns '%s'",
			host ? Net::IPv4::ntoa(host->ip) : "(null)", hostname);
	return host ? host->ip : 0;
}

const char* DomainResolver::ddns(uint32_t ip) THROWS {
	const char* hostname = NULL;
	ResolvItem* host = *_ipToName.get(ip);
	if (host)
		hostname = host->name;
	Utils::Log::d("%s <-- ddns %s", hostname, Net::IPv4::ntoa(ip));
	return hostname;
}

bool DomainResolver::onHttpRequest(Net::HttpRequest& request,
		Utils::JSONObject& response) THROWS {
	Utils::String path = request.getPath();
	if (path == "/dns.json") {
		const char* host = request.getQueryString("host");
		uint32_t ip = dns(request.getRemoteAddr().ip, host);
		response.put("Status", 0);
		response.put("Message", "OK");
		response.put("Host", host);
		response.put("IP", ip == 0 ? NULL : Net::IPv4::ntoa(ip));
		return true;
	} else if (path == "/ddns.json") {
		const char* ip = request.getQueryString("ip");
		const char* host = ddns(Net::IPv4::aton(ip));
		response.put("Status", 0);
		response.put("Message", "OK");
		response.put("IP", ip);
		response.put("Host", host);
		return true;
	}
	return HttpService::onHttpRequest(request, response);
}

bool DomainResolver::onHttpRequest(Net::HttpRequest& request,
		Net::HttpResponse& response) THROWS {
	Utils::String path = request.getPath();
	if (path == "/debug/resolve.html") {
		response.setStatus(200, "OK");
		response.setContentType("text/html");
		response.printf(
				"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />");
		response.printf("<title>Domain Resolver</title>");
		response.printf(
				"<table border=\"1\" bordercolor=\"lightgrey\" style=\"border-collapse: collapse\">");
		for (ResolvItemByName* item = _nameToIp.min(); item;
				item = _nameToIp.bigger(item)) {
			response.printf("<tr>");
			response.printf("<td>%s</td>", (*item)->name.sz());
			response.printf("<td>%s</td>", Net::IPv4::ntoa((*item)->ip));
			response.printf("</tr>");
		}
		response.printf("</table>");
		return true;
	}
	return HttpService::onHttpRequest(request, response);
}

}
