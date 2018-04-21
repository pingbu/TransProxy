#include "Base/Debug.h"
#include "Base/Utils.h"
#include "Net/HttpClient.h"
#include "HTTP.h"

#pragma once

namespace TransProxy {

class Config: public HttpService {
	Utils::String _workDir, _rulesFile, _proxyListFile, _directListFile;
	Utils::IniFile _ini;
	Net::HttpClient _httpClient;
	Net::HttpClient::Request* _httpRequest;

	struct _: Net::HttpClient::Listener {
		Config* _this;
		int fd;
		_(Config* thiz) :
				_this(thiz), fd(-1) {
		}
		void _close() {
			if (fd != -1) {
				::close(fd);
				fd = -1;
			}
			_this->_httpRequest->close();
			_this->_httpRequest = NULL;
		}
		void onResponseHeader(Net::HttpClient::Response* response) THROWS {
			if (response->getStatus() == 200) {
				Utils::Log::i("%d %s <-- %s", response->getStatus(),
						response->getMessage(), _this->_getRulesURL());
				Utils::String fn = _this->_rulesFile + ".tmp";
				fd = ::open(fn, O_CREAT | O_TRUNC | O_WRONLY);
				if (fd == -1) {
					Utils::Log::w("FAILED to open temporary rules file '%s'",
							fn.sz());
					_close();
				}
			} else {
				Utils::Log::w("Response %d %s", response->getStatus(),
						response->getMessage());
				_close();
			}
		}
		void onResponseContent(const void* data, size_t bytes) THROWS {
			Utils::Log::v("Response Content Data %u bytes", bytes);
			if (::write(fd, data, bytes) != (ssize_t) bytes) {
				Utils::Log::w("FAILED to write rules file");
				_close();
			}
		}
		void onResponseCompleted() THROWS {
			Utils::Log::v("Response Completed");
			_close();
			_this->_updateRulesFile();
		}
		void onHttpClientError(Utils::Exception* e) THROWS {
			Utils::Log::w("HttpClient Error");
			e->print();
			_close();
		}
	} _httpRequestListener;

	static void _restoreFile(const char* file);

	const char* _getRulesURL();
	const char* _getRulesFormat();
	void _updateRulesFile() THROWS;

	const char* _getCustomMask();
	const char* _getCustomClientIP();
	const char* _getCustomServerIP();
	const char* _getCustomVipMin();
	const char* _getCustomVipMax();
	const char* _getCustomAgentMin();
	const char* _getCustomAgentMax();

public:
	Config(const char* workDir);
	virtual ~Config() {
		Utils::Log::e("~Config");
	}

	bool onHttpRequest(Net::HttpRequest& request, Utils::JSONObject& response)
			THROWS;

	const char* getWorkDir() const {
		return _workDir;
	}

	const char* getProxyURL();
	const char* getUpDnsURL();

	const char* getRulesFile() const {
		return _rulesFile;
	}
	const char* getCustomProxyListFile() const {
		return _proxyListFile;
	}
	const char* getCustomDirectListFile() const {
		return _directListFile;
	}

	int getNetworkType();
	const char* getMask();
	const char* getClientIP();
	const char* getServerIP();
	const char* getVipMin();
	const char* getVipMax();
	const char* getAgentMin();
	const char* getAgentMax();
};

}
