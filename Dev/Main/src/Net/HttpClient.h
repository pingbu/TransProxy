#include "DnsClient.h"
#include "TcpClient.h"
#include "SSLConnection.h"

#pragma once

namespace Net {

class HttpClient {
public:
	struct Request {
	protected:
		virtual ~Request() {
		}
	public:
		virtual void send(void* data = NULL, size_t bytes = 0,
				const char* type = NULL) THROWS = 0;
		virtual void close() THROWS = 0;
	};

	struct Response {
		virtual ~Response() {
		}
		virtual int getStatus() const = 0;
		virtual const char* getMessage() const = 0;
		virtual const char* getContentType() const = 0;
	};

	struct Listener {
		virtual ~Listener() {
		}
		virtual void onResponseHeader(Response* response) THROWS = 0;
		virtual void onResponseContent(const void* data, size_t bytes)
				THROWS = 0;
		virtual void onResponseCompleted() THROWS = 0;
		virtual void onHttpClientError(Utils::Exception* e) THROWS = 0;
	};

private:
	DnsClient* _dns;
	TcpClientFactory* _factory;

public:
	HttpClient(DnsClient* dns, TcpClientFactory* factory) :
			_dns(dns), _factory(factory) {
	}

	Request* openRequest(const char* url, Listener* listener) THROWS {
		return openRequest("GET", url, listener);
	}
	Request* openRequest(const char* method, const char* url,
			Listener* listener) THROWS;
};

}
