#include <time.h>
#include "Base/Debug.h"
#include "Base/Utils.h"
#include "DnsAgent.h"
#include "UDP.h"

#pragma once

namespace TransProxy {

class DomainResolver;

class DNS: public HttpService, DnsAgentListener, Net::UdpPeerListener {
	enum {
		MAX_LOGS = 1000
	};

	struct _AgentRequest: Utils::TimerListener {
		DNS* _this;
		Net::IPv4::SockAddr _addr;
		int _id;
		_AgentRequest(DNS* thiz, Net::IPv4::SockAddr addr, void* data,
				size_t bytes) :
				_this(thiz), _addr(addr) {
			_id = thiz->_dnsAgent->query(this, data, bytes);
		}
		~_AgentRequest() {
			if (_id >= 0)
				_this->_dnsAgent->cancel(_id);
		}

		// Utils::TimerListener
		void onTimeout() {
			delete this;
		}
		void onTimerError(Utils::Exception* e) THROWS {
			THROW(e);
		}
	};

	struct _LogItem: Utils::ListItem {
		time_t time;
		uint32_t client;
		Utils::String hostname;
		uint32_t ip;
		_LogItem(uint32_t client, const char* hostname, uint32_t ip) :
				time(::time(NULL)), client(client), hostname(hostname), ip(ip) {
		}
	};

	uint32_t _serverIP;
	DomainResolver* _domainResolver;
	DnsAgent* _dnsAgent;
	Net::UdpPeer* _dnsServer;
	Utils::List<_LogItem> _logs;
	size_t _count;

	void _log(uint32_t client, const char* hostname, uint32_t ip) {
		if (_logs.size() >= MAX_LOGS)
			_logs.remove(_logs.first());
		_logs.insertTail(new _LogItem(client, hostname, ip));
		++_count;
	}

	// UdpPeerListener
	void onReceived(Net::IPv4::SockAddr addr, void* data, size_t bytes) THROWS;
	void onError(Utils::Exception* e) THROWS {
		e->print();
	}

	// DnsAgentListener {
	void onDnsAgentResponse(void* user, const void* data, size_t bytes) THROWS {
		_AgentRequest* req = (_AgentRequest*) user;
		_dnsServer->send(req->_addr, data, bytes);
		req->_id = -1;
		delete req;
	}

public:
	DNS(UDP* udp, Net::IPv4::ServiceAddr bindAddr,
			Net::IPv4::ServiceAddr upDnsAddr, DomainResolver* domainResolver) :
			_serverIP(bindAddr.sockAddr.ip), _domainResolver(domainResolver), _count(
					0) THROWS {
		Utils::Log::i("DNS initializing...");
		THROW_IF(
				upDnsAddr.proto != Net::PROTO_UDP
						|| bindAddr.proto != Net::PROTO_UDP,
				new Utils::Exception("DNS only support UDP, '%s' unsupported!", upDnsAddr.toString().sz()));

		_dnsAgent = new DnsAgent(this, upDnsAddr.sockAddr);
		Utils::Log::i("Up DNS set to %s", upDnsAddr.toString().sz());

		if (bindAddr.sockAddr.port == 0)
			bindAddr.sockAddr.port = 53;
		_dnsServer = udp->bind(bindAddr.sockAddr, this);
		Utils::Log::i("DNS bound at port %u", _dnsServer->getPort());
	}
	virtual ~DNS() {
		Utils::Log::e("~DNS");
	}

	size_t getResolveCount() const {
		return _count;
	}

	// HttpService
	bool onHttpRequest(Net::HttpRequest& request, Utils::JSONObject& response)
			THROWS;
};

}
