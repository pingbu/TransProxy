#include <stdarg.h>
#include <stdlib.h>
#include "Base/String.h"
#include "Base/Debug.h"
#include "TcpServer.h"

#pragma once

namespace Net {

struct HttpRequest {
	virtual ~HttpRequest() {
	}
	virtual IPv4::SockAddr getRemoteAddr() const = 0;
	virtual const char* getMethod() const = 0;
	virtual const char* getHost() const = 0;
	virtual const char* getPath() const = 0;
	virtual const char* getQueryString(const char* name) const = 0;
	virtual const char* getContentType() const = 0;
	virtual const void* getContent() const = 0;
	virtual size_t getContentLength() const = 0;
};

struct HttpResponse {
	virtual ~HttpResponse() {
	}
	virtual void setStatus(int code, const char* msg) = 0;
	virtual void setContentType(const char* contentType) = 0;
	virtual void write(const void* data, size_t bytes) = 0;
	virtual void writeFile(const char* path) = 0;
	void print(const char* s) {
		write(s, ::strlen(s));
	}
	void println(const char* s) {
		printf("%s\n", s);
	}
	void printf(const char* fmt, ...) {
		va_list ap;
		va_start(ap, fmt);
		Utils::String s = Utils::String::formatV(fmt, ap);
		va_end(ap);
		write(s.sz(), s.length());
	}
	virtual void flush() = 0;
	virtual void end() = 0;
};

struct HttpServerListener {
	virtual ~HttpServerListener() {
	}
	virtual void onHttpRequest(HttpRequest& request, HttpResponse& response)
			THROWS = 0;
};

class HttpServer: TcpServerListener {
	TcpServer* _server;
	HttpServerListener* _listener;

	TcpConnectionListener* onTcpServerConnected(TcpConnection* conn) THROWS;

	void onTcpServerError(Utils::Exception* e) {
		e->print();
		::abort();
	}

public:
	HttpServer(TcpServerFactory* serverFactory, uint16_t port,
			HttpServerListener* listener) :
			_listener(listener) {
		if (port == 0)
			port = 80;
		_server = serverFactory->tcp_listen(port, this);
	}
	~HttpServer() {
		delete _server;
	}

	uint16_t getPort() const {
		return _server->getPort();
	}
};

}
