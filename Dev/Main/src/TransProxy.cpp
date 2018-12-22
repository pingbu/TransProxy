#define LOG_TAG "TransProxy"

#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "Base/Utils.h"
#include "Base/Debug.h"
#include "Net/Tun.h"
#include "TransProxy/Config.h"
#include "TransProxy/MallocHTTP.h"
#include "TransProxy/DomainResolver.h"
#include "TransProxy/DomainRules.h"
#include "TransProxy/CustomList.h"
#include "TransProxy/TunMac.h"
#include "TransProxy/IPv4.h"
#include "TransProxy/Ping.h"
#include "TransProxy/UDP.h"
#include "TransProxy/TCP.h"
#include "TransProxy/DNS.h"
#include "TransProxy/HTTP.h"
#include "TransProxy/TransTCP.h"

using namespace TransProxy;

static const char* PRODUCT = "TransProxy";
static const char* VERSION = "1.0.1";
static int REVISION = 1;

static Utils::String getExeDir() {
	char dir[256];
	THROW_IF(::readlink("/proc/self/exe", dir, sizeof(dir)) <= 0,
			new Utils::Exception("FAILED to getExeDir"));
	char* p = ::strrchr(dir, '/');
	THROW_IF(p == NULL, new Utils::Exception("FAILED to getExeDir"));
	*p = '\0';
	Utils::Log::d("WorkDir=%s", dir);
	return dir;
}

static DNS* _dns;
static TransTCP* _transTCP;

static struct _: HttpService, Utils::TimerListener {
	time_t _startTime;
	Utils::Timer _timer;

	_() :
			_startTime(::time(NULL)), _timer("Main", this) {
	}

	void onTimeout() {
		::exit(1);
	}

	void onTimerError(Utils::Exception* e) THROWS {
		THROW(e);
	}

	bool onHttpRequest(Net::HttpRequest& request, Utils::JSONObject& response)
			THROWS {
		Utils::String path = request.getPath();
		if (path == "/version.json") {
			response.put("Status", 0);
			response.put("Message", "OK");
			response.put("Product", PRODUCT);
			response.put("Version", VERSION);
			response.put("Revision", REVISION);
			return true;
		} else if (path == "/status.json") {
			response.put("Status", 0);
			response.put("Message", "OK");
			response.put("RunDuration",
					Utils::formatTimeSpan(::time(NULL) - _startTime).sz());
			response.put("ResolveCount", (int) _dns->getResolveCount());
			response.put("ConnectionCount",
					(int) _transTCP->getConnectionCount());
			response.put("MaxConnectionCount",
					(int) _transTCP->getMaxConnectionCount());
			response.put("TotalUpData",
					Utils::formatSize(_transTCP->getTotalUpBytes()).sz());
			response.put("TotalDownData",
					Utils::formatSize(_transTCP->getTotalDownBytes()).sz());
			return true;
		} else if (path == "/reboot.json") {
			_timer.setTimeout(3000);
			response.put("Status", 0);
			response.put("Message", "OK");
			return true;
		}
		return HttpService::onHttpRequest(request, response);
	}
} _mainHttpService;

static Config config(getExeDir());

static void showBanner() {
	::fprintf(stderr, "\033[0m"
			"============================================================\n"
			" TransProxy v%s\n"
			"------------------------------------------------------------\n"
			" Service DNS: %s\n"
			" Manager URL: http://transproxy.cn/\n"
			"============================================================\n",
			VERSION, config.getServerIP());
}

static void run() THROWS {
	Utils::Looper::prepare();
	Utils::String workDir = getExeDir();

	DomainResolver* domainResolver = new DomainResolver(config.getVipMin(),
			workDir);

	DomainRules* domainRules = new DomainRules(config.getRulesFile());
	domainResolver->addRules(domainRules);
	Utils::Log::i("DomainResolver <--addRules-- DomainRules");

	CustomList* customList = new CustomList(config.getCustomProxyListFile(),
			config.getCustomDirectListFile());
	domainResolver->addRules(customList);
	Utils::Log::i("DomainResolver <--addRules-- CustomList");

	TunMac* tunMac = new TunMac(config.getClientIP(), config.getMask());

	IPv4* ipv4 = new IPv4(tunMac);
	tunMac->addProtocol(ipv4);

	Ping* ping = new Ping(ipv4);
	UDP* udp = new UDP(ipv4);
	TCP* tcp = new TCP(ipv4);
	_transTCP = new TransTCP(ipv4, config.getAgentMin(), domainResolver,
			config.getProxyURL(), config.getClientIP());
	ipv4->addProtocol(ping);
	ipv4->addProtocol(udp);
	ipv4->addProtocol(tcp);
	ipv4->addProtocol(_transTCP);

	domainResolver->addRules(_transTCP);
	Utils::Log::i("DomainResolver <--addRules-- TransTCP");

	Utils::String dnsUrl = Utils::String("udp://") + config.getServerIP();
	_dns = new DNS(udp, dnsUrl.sz(), config.getUpDnsURL(), domainResolver);

	MallocHTTP* mallocHTTP = new MallocHTTP();

	HTTP* http = new HTTP(tcp->bind(Net::IPv4::aton(config.getServerIP())), 80,
			workDir + "/www");
	http->addService(&_mainHttpService);
	http->addService(&config);
	http->addService(domainResolver);
	http->addService(_transTCP);
	http->addService(_dns);
	http->addService(mallocHTTP);

	MallocHTTP::startLog();
	Utils::Looper::loop();
}

int main(int argc, char* argv[]) {
	bool daemon = false;
	int logLevel = -1;
	for (int i = 1; i < argc; ++i)
		if (::strcmp(argv[i], "-d") == 0
				|| ::strcmp(argv[i], "--daemon") == 0) {
			daemon = true;
		} else if (::strncmp(argv[i], "-l=", 3) == 0
				|| ::strncmp(argv[i], "--log-level=", 12) == 0) {
			logLevel = ::atoi(::strchr(argv[i], '=') + 1);
		}

	if (daemon) {
		::daemon(0, 0);
		for (;;) {
			int r = ::fork();
			if (r == 0) {
				// in child process
				break;
			} else {
				// in parent process
				if (r == -1)
					::sleep(1);
				else
					::wait(NULL);
			}
		}
	} else {
		showBanner();
	}

	if (logLevel >= 0)
		Utils::Log::setLevel(logLevel);

	TRY{ run();}
	CATCH (e){ e->print(); ::abort();}
	return 0;
}
