#define LOG_TAG "DNS"

#include <alloca.h>
#include "Base/Debug.h"
#include "Base/Utils.h"
#include "DomainResolver.h"
#include "DNS.h"

namespace TransProxy {

void DNS::onReceived(Net::IPv4::SockAddr addr, void* data, size_t bytes)
		THROWS {
	uint16_t QDCOUNT = ntohs(((uint16_t*) data)[2]);
	uint16_t ANCOUNT = ((uint16_t*) data)[3];
	uint16_t NSCOUNT = ((uint16_t*) data)[4];
	uint16_t ARCOUNT = ((uint16_t*) data)[5];
	//Utils::log("QDCOUNT=%u, ANCOUNT=%u, NSCOUNT=%u, ARCOUNT=%u",
	//		QDCOUNT, ANCOUNT, NSCOUNT, ARCOUNT);
	if (QDCOUNT == 1 && ANCOUNT == 0 && NSCOUNT == 0 && ARCOUNT == 0) {
		char* hostname_buf = (char*) ::alloca(bytes - 16);
		uint8_t* a = (uint8_t*) data + 12;
		for (char* b = hostname_buf;;) {
			uint8_t l = *a++;
			if (l == 0) {
				*b++ = '\0';
				break;
			}
			*b++ = '.';
			::memcpy(b, a, l);
			a += l;
			b += l;
		}
		const char* hostname = hostname_buf + 1;
		uint32_t ip;
		if (::strcmp(hostname, "transproxy.cn") == 0)
			ip = _serverIP;
		else
			ip = _domainResolver->dns(addr.ip, hostname);
		uint16_t QTYPE = ntohs(*(uint16_t*) a);
		uint16_t QCLASS = ntohs(*(uint16_t*) (a + 2));
		//Utils::log("QTYPE=%u, QCLASS=%u", QTYPE, QCLASS);
		if (QTYPE == 1 && QCLASS == 1) { // A记录
			if (ip != 0) {
				size_t responseSize = bytes + 16;
				uint8_t* response = (uint8_t*) ::alloca(responseSize);
				::memcpy(response, data, bytes);
				uint16_t& FLAGS = *(uint16_t*) (response + 2);
				FLAGS = FLAGS | htons(0x8080); // QR=1, RA=1
				*(uint16_t*) (response + 6) = htons(1); // ANCOUNT=1
				uint8_t* p = response + bytes;
				*(uint16_t*) p = htons(0xC00C); // 引用请求中的域名
				*(uint16_t*) (p + 2) = htons(1); // 1-A记录
				*(uint16_t*) (p + 4) = htons(1); // 1-Internet数据
				*(uint32_t*) (p + 6) = 0;
				*(uint16_t*) (p + 10) = htons(4);
				*(uint32_t*) (p + 12) = htonl(ip);
				_dnsServer->send(addr, response, responseSize);
				_log(addr.ip, hostname, ip);
				return;
			}
			_log(addr.ip, hostname, 0);
		} else if (QTYPE == 28 && QCLASS == 1) { // AAAA记录
			if (ip != 0) {
				uint8_t* response = (uint8_t*) ::alloca(bytes);
				::memcpy(response, data, bytes);
				uint16_t& FLAGS = *(uint16_t*) (response + 2);
				FLAGS = FLAGS | 0x8080; // QR=1, RA=1
				_dnsServer->send(addr, response, bytes);
				return;
			}
		}
	}
	new _AgentRequest(this, addr, data, bytes);
}

// DNS::HttpService
class IpSetItem: public Utils::MapItem<uint32_t> {
	uint32_t _ip;
public:
	IpSetItem(uint32_t ip) {
		_ip = ip;
	}
	operator uint32_t() const {
		return _ip;
	}
	uint32_t getKey() const {
		return _ip;
	}
	Utils::String getKeyString() const {
		return Net::IPv4::ntoa(_ip);
	}
};

bool DNS::onHttpRequest(Net::HttpRequest& request, Utils::JSONObject& response)
		THROWS {
	Utils::String path = request.getPath();
	if (path == "/dnslog.json") {
		uint32_t client = request.getRemoteAddr().ip;
		Utils::Map<uint32_t, IpSetItem> ips;
		ips.add(new IpSetItem(client));
		const char* client_ = request.getQueryString("client");
		if (client_) {
			uint32_t client__ = Net::IPv4::aton(client_);
			if (client__ != 0 && client__ != client) {
				client = client__;
				ips.add(new IpSetItem(client));
			}
		}
		for (_LogItem* log = _logs.first(); log; log = _logs.next(log))
			if (!ips.get(log->client))
				ips.add(new IpSetItem(log->client));

		Utils::JSONArray* clients = new Utils::JSONArray();
		for (IpSetItem* item = ips.min(); item; item = ips.bigger(item))
			clients->put(Net::IPv4::ntoa(*item));

		Utils::JSONArray* logs = new Utils::JSONArray();
		for (_LogItem* item = _logs.last(); item; item = _logs.prev(item))
			if (item->client == client) {
				Utils::JSONObject* log = new Utils::JSONObject();
				Utils::String t = Utils::formatTime(item->time);
				log->put("Time", t.sz());
				log->put("Host", item->hostname.sz());
				log->put("IP",
						item->ip == 0 ? NULL : Net::IPv4::ntoa(item->ip));
				logs->put(log);
			}

		response.put("Status", 0);
		response.put("Message", "OK");
		response.put("Clients", clients);
		response.put("CurrentClient", Net::IPv4::ntoa(client));
		response.put("Logs", logs);
		return true;
	}
	return HttpService::onHttpRequest(request, response);
}

}
