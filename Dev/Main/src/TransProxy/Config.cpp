#define LOG_TAG "Config"

#include <fcntl.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <unistd.h>
#include "Base/Debug.h"
#include "Net/DnsClientSystem.h"
#include "Net/TcpClientDirect.h"
#include "Config.h"

namespace TransProxy {

static const char type10_mask[] = "255.0.0.0";
static const char type10_clientIP[] = "10.0.0.1";
static const char type10_serverIP[] = "10.10.10.10";
static const char type10_vipMin[] = "10.128.0.1";
static const char type10_vipMax[] = "10.255.255.254";
static const char type10_agentMin[] = "10.100.0.1";
static const char type10_agentMax[] = "10.127.255.255";

static const char type100_mask[] = "255.192.0.0";
static const char type100_clientIP[] = "100.64.0.1";
static const char type100_serverIP[] = "100.100.100.100";
static const char type100_vipMin[] = "100.101.0.1";
static const char type100_vipMax[] = "100.127.255.255";
static const char type100_agentMin[] = "100.72.0.1";
static const char type100_agentMax[] = "100.99.255.255";

static Utils::String _readFile(const char* file) {
	FILE* fp = ::fopen(file, "rb");
	if (!fp)
		return "";
	::fseek(fp, 0, SEEK_END);
	size_t l = (size_t) ::ftell(fp);
	char t[l + 1];
	::fseek(fp, 0, SEEK_SET);
	l = (size_t) ::fread(t, 1, l, fp);
	::fclose(fp);
	t[l] = '\0';
	return t;
}

static void _saveFile(const char* file, const char* content) {
	Utils::String tmpFile(file), bakFile(file);
	tmpFile += ".tmp";
	bakFile += ".bak";
	FILE* fp = ::fopen(tmpFile, "wb");
	::fwrite(content, 1, ::strlen(content), fp);
	::fclose(fp);
	::rename(file, bakFile);
	::rename(tmpFile, file);
	::remove(bakFile);
}

void Config::_restoreFile(const char* file) {
	Utils::String tmpFile(file), bakFile(file);
	tmpFile += ".tmp";
	bakFile += ".bak";
	::remove(tmpFile);
	if (::access(file, F_OK) != 0 && ::access(bakFile, F_OK) == 0) {
		::rename(bakFile, file);
		Utils::Log::w("Restored file '%s'", file);
	} else {
		::remove(bakFile);
	}
}

Config::Config(const char* workDir) :
		_workDir(workDir), _rulesFile(_workDir + "/domain_rules.txt"), _proxyListFile(
				_workDir + "/domain_proxy.txt"), _directListFile(
				_workDir + "/domain_direct.txt"), _ini(
				_workDir + "/transproxy.conf"), _httpClient(
		new Net::DnsClientSystem(),
		new Net::TcpClientDirect::Factory()), _httpRequest(NULL), _httpRequestListener(
				this) {
	_restoreFile(_rulesFile);
	_restoreFile(_proxyListFile);
	_restoreFile(_directListFile);
}

const char* Config::getProxyURL() {
	return _ini.getValue("Proxy", "proxy", "http://192.168.0.1:8080");
}
const char* Config::getUpDnsURL() {
	return _ini.getValue("Proxy", "up.dns", "udp://114.114.114.114");
}
const char* Config::_getRulesURL() {
	return _ini.getValue("Proxy", "rules.url",
			"https://raw.githubusercontent.com/gfwlist/gfwlist/master/gfwlist.txt");
}
const char* Config::_getRulesFormat() {
	return _ini.getValue("Proxy", "rules.format", "Base64");
}
int Config::getNetworkType() {
	return ::atoi(_ini.getValue("Network", "type", "100"));
}
const char* Config::_getCustomMask() {
	return _ini.getValue("Network", "mask", type100_mask);
}
const char* Config::_getCustomClientIP() {
	return _ini.getValue("Network", "client.ip", type100_clientIP);
}
const char* Config::_getCustomServerIP() {
	return _ini.getValue("Network", "server.ip", type100_serverIP);
}
const char* Config::_getCustomVipMin() {
	return _ini.getValue("Network", "vip.min", type100_vipMin);
}
const char* Config::_getCustomVipMax() {
	return _ini.getValue("Network", "vip.max", type100_vipMax);
}
const char* Config::_getCustomAgentMin() {
	return _ini.getValue("Network", "agent.min", type100_agentMin);
}
const char* Config::_getCustomAgentMax() {
	return _ini.getValue("Network", "agent.max", type100_agentMax);
}

const char* Config::getMask() {
	int type = getNetworkType();
	return type == 0 ? _getCustomMask() :
			type == 10 ? type10_mask : type100_mask;
}
const char* Config::getClientIP() {
	int type = getNetworkType();
	return type == 0 ? _getCustomClientIP() :
			type == 10 ? type10_clientIP : type100_clientIP;
}
const char* Config::getServerIP() {
	int type = getNetworkType();
	return type == 0 ? _getCustomServerIP() :
			type == 10 ? type10_serverIP : type100_serverIP;
}
const char* Config::getVipMin() {
	int type = getNetworkType();
	return type == 0 ? _getCustomVipMin() :
			type == 10 ? type10_vipMin : type100_vipMin;
}
const char* Config::getVipMax() {
	int type = getNetworkType();
	return type == 0 ? _getCustomVipMax() :
			type == 10 ? type10_vipMax : type100_vipMax;
}
const char* Config::getAgentMin() {
	int type = getNetworkType();
	return type == 0 ? _getCustomAgentMin() :
			type == 10 ? type10_agentMin : type100_agentMin;
}
const char* Config::getAgentMax() {
	int type = getNetworkType();
	return type == 0 ? _getCustomAgentMax() :
			type == 10 ? type10_agentMax : type100_agentMax;
}

void Config::_updateRulesFile() THROWS {
	Utils::String fTmp = _rulesFile + ".tmp";
	if (::strcasecmp(_getRulesFormat(), "Base64") == 0) {
		Utils::String fTmp2 = _rulesFile + ".tmp2";
		BIO* bioTmp = ::BIO_new_file(fTmp, "rb");
		if (bioTmp == NULL) {
			THROW(
					new Utils::Exception("FAILED to open temporary rules file '%s'", fTmp.sz()));
		}
		BIO* bioTmp2 = ::BIO_new_file(fTmp2, "wb");
		if (bioTmp2 == NULL) {
			::BIO_free_all(bioTmp);
			THROW(
					new Utils::Exception("FAILED to create decoded temporary rules file '%s'", fTmp2.sz()));
		}
		bioTmp = ::BIO_push(::BIO_new(::BIO_f_base64()), bioTmp);
		for (uint8_t buf[1024];;) {
			int l = ::BIO_read(bioTmp, buf, sizeof(buf));
			if (l <= 0)
				break;
			int r = ::BIO_write(bioTmp2, buf, l);
			if (r != l) {
				::BIO_free_all(bioTmp2);
				::BIO_free_all(bioTmp);
				THROW(
						new Utils::Exception("FAILED to write decoded temporary rules file '%s'", fTmp2.sz()));
			}
		}
		BIO_flush(bioTmp2);
		::BIO_free_all(bioTmp2);
		::BIO_free_all(bioTmp);
		::remove(fTmp);
		::rename(fTmp2, fTmp);
		Utils::Log::i("Base64 temporary rules file decoded.");
	}
	Utils::String fBak = _rulesFile + ".bak";
	::rename(_rulesFile, fBak);
	::rename(fTmp, _rulesFile);
	::remove(fBak);
}

bool Config::onHttpRequest(Net::HttpRequest& request,
		Utils::JSONObject& response) THROWS {
	Utils::String path = request.getPath();
	if (path == "/config.json") {
		Utils::String proxyList = _readFile(_proxyListFile);
		Utils::String directList = _readFile(_directListFile);

		Utils::JSONObject* networkCustom = new Utils::JSONObject();
		networkCustom->put("Mask", _getCustomMask());
		networkCustom->put("ClientIP", _getCustomClientIP());
		networkCustom->put("ServerIP", _getCustomServerIP());
		networkCustom->put("VipMin", _getCustomVipMin());
		networkCustom->put("VipMax", _getCustomVipMax());
		networkCustom->put("AgentMin", _getCustomAgentMin());
		networkCustom->put("AgentMax", _getCustomAgentMax());

		response.put("Status", 0);
		response.put("Message", "OK");
		response.put("Proxy", getProxyURL());
		response.put("UpDNS", getUpDnsURL());
		response.put("RulesURL", _getRulesURL());
		response.put("RulesFormat", _getRulesFormat());
		response.put("CustomProxyList", proxyList.sz());
		response.put("CustomDirectList", directList.sz());
		response.put("NetworkType", getNetworkType());
		response.put("NetworkCustom", networkCustom);
		return true;

	} else if (path == "/config-save.json") {
		Utils::JSONObject* req = new Utils::JSONObject(
				(const char*) request.getContent());

		_ini.setValue("Proxy", "proxy", req->getString("Proxy"));
		_ini.setValue("Proxy", "up.dns", req->getString("UpDNS"));
		_ini.setValue("Proxy", "rules.url", req->getString("RulesURL"));
		_ini.setValue("Proxy", "rules.format", req->getString("RulesFormat"));
		_ini.setValue("Network", "type",
				Utils::String::format("%u", req->getInt("NetworkType")));
		Utils::JSONObject* networkCustom = req->getJSONObject("NetworkCustom");
		_ini.setValue("Network", "mask", networkCustom->getString("Mask"));
		_ini.setValue("Network", "client.ip",
				networkCustom->getString("ClientIP"));
		_ini.setValue("Network", "server.ip",
				networkCustom->getString("ServerIP"));
		_ini.setValue("Network", "vip.min", networkCustom->getString("VipMax"));
		_ini.setValue("Network", "vip.max", networkCustom->getString("VipMax"));
		_ini.setValue("Network", "agent.min",
				networkCustom->getString("AgentMin"));
		_ini.setValue("Network", "agent.max",
				networkCustom->getString("AgentMax"));
		_ini.save();

		_saveFile(_proxyListFile, req->getString("CustomProxyList"));
		_saveFile(_directListFile, req->getString("CustomDirectList"));

		req->release();

		response.put("Status", 0);
		response.put("Message", "OK");
		return true;

	} else if (path == "/config-update-rules-file-start.json") {
		if (!_httpRequest) {
			_httpRequest = _httpClient.openRequest(_getRulesURL(),
					&_httpRequestListener);
			_httpRequest->send();

			response.put("Status", 0);
			response.put("Message", "OK");
		} else {
			response.put("Status", -1);
			response.put("Message", "Already updating...");
		}
		return true;

	} else if (path == "/config-update-rules-file-check.json") {
		struct stat rulesStat;
		if (::stat(_rulesFile, &rulesStat) == 0) {
			response.put("Status", 0);
			response.put("Message", "OK");
			response.put("LastUpdate", ::ctime(&rulesStat.st_mtim.tv_sec));
			response.put("Updating", _httpRequest != NULL);
		} else {
			response.put("Status", -1);
			response.put("Message", "FAILED to stat file");
		}
		return true;
	}
	return HttpService::onHttpRequest(request, response);
}

}
