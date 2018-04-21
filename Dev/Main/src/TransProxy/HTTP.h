#include <stddef.h>
#include "Base/Debug.h"
#include "Base/Utils.h"
#include "Net/TcpServer.h"
#include "Net/HttpServer.h"

#pragma once

namespace TransProxy {

class HttpService {
	friend class HTTP;
	HttpService* _next;

public:
	virtual ~HttpService() {
	}
	virtual bool onHttpRequest(Net::HttpRequest& request,
			Utils::JSONObject& response) THROWS {
		return false;
	}
	virtual bool onHttpRequest(Net::HttpRequest& request,
			Net::HttpResponse& response) THROWS {
		Utils::JSONObject* json = new Utils::JSONObject();
		bool r = onHttpRequest(request, *json);
		if (r) {
			response.setStatus(200, "OK");
			response.setContentType("text/json");
			response.print(json->toString());
		}
		json->release();
		return r;
	}
};

class HTTP: Net::HttpServerListener {
	Net::HttpServer _server;
	Utils::String _wwwroot;
	HttpService* _services = NULL;

	// Net::HttpServerListener
	void onHttpRequest(Net::HttpRequest& request, Net::HttpResponse& response)
			THROWS {
		for (HttpService* service = _services; service; service =
				service->_next)
			if (service->onHttpRequest(request, response))
				return;

		Utils::String path = _wwwroot + request.getPath();
		if (path.endsWith("/"))
			path += "index.html";
		response.writeFile(path);
	}

public:
	HTTP(Net::TcpServerFactory* serverFactory, uint16_t port,
			const char* rootDir) :
			_server(serverFactory, port, this), _wwwroot(rootDir) {
		Utils::Log::i("HTTP initializing...");
		Utils::Log::i("HTTP wwwroot: %s", _wwwroot.sz());
		Utils::Log::i("HTTP listening at port %u", getPort());
	}
	~HTTP() {
		Utils::Log::e("~HTTP");
	}

	uint16_t getPort() const {
		return _server.getPort();
	}
	void addService(HttpService* service) {
		service->_next = _services;
		_services = service;
	}
};

}
