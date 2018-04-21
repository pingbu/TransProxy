#pragma once

#include "Net/UdpPeerDirect.h"

namespace TransProxy {

struct DnsAgentListener {
	virtual ~DnsAgentListener() {
	}
	virtual void onDnsAgentResponse(void* user, const void* data, size_t bytes)
			THROWS = 0;
};

class DnsAgent {
	enum {
		MIN_ID = 1, MAX_ID = 5000
	};
	struct {
		void* user;
		uint16_t id;
		bool valid;
	} _items[MAX_ID - MIN_ID + 1];
	uint16_t _id;

	DnsAgentListener* _listener;
	Net::IPv4::SockAddr _addr;
	Net::UdpPeer* _udpPeer;

	struct _UdpPeerListener: Net::UdpPeerListener {
		DnsAgent* _this;
		_UdpPeerListener(DnsAgent* thiz) :
				_this(thiz) {
		}
		void onReceived(Net::IPv4::SockAddr addr, void* data, size_t bytes)
				THROWS {
			uint16_t id = ntohs(*(uint16_t*) data);
			if (_this->_items[id].valid) {
				*(uint16_t*) data = htons(_this->_items[id].id);
				_this->_listener->onDnsAgentResponse(_this->_items[id].user,
						data, bytes);
			}
		}
		void onError(Utils::Exception* e) THROWS {
			THROW(e);
		}
	}* _udpPeerListener;

public:
	DnsAgent(DnsAgentListener* listener, Net::IPv4::SockAddr addr) :
			_id(MIN_ID), _listener(listener), _addr(addr) {
		Utils::Log::i("DNS agent initializing...");
		if (_addr.port == 0)
			_addr.port = 53;
		_udpPeerListener = new _UdpPeerListener(this);
		_udpPeer = new Net::UdpPeerDirect(_udpPeerListener);
	}
	virtual ~DnsAgent() {
		delete _udpPeer;
		delete _udpPeerListener;
	}

	int query(void* user, void* data, size_t bytes) {
		int id = _id++;
		_items[id].user = user;
		_items[id].id = ntohs(*(uint16_t*) data);
		_items[id].valid = true;
		*(uint16_t*) data = htons(id);
		_udpPeer->send(_addr, data, bytes);
		return id;
	}
	void cancel(int id) {
		_items[id].valid = false;
	}
};

}
