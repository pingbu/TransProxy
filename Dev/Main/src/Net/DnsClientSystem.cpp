#define LOG_TAG  "DnsClientSystem"

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include "Base/Debug.h"
#include "Base/Utils.h"
#include "IPv4.h"
#include "DnsClientSystem.h"

namespace Net {

class DnsQuerySystem: public DnsQuery {
	Utils::String _hostname;
	DnsQueryListener* _listener;
	pthread_spinlock_t _lock;
	int _event, _selector;
	pthread_t _tid;
	uint32_t _ip;

	struct _: Utils::FDListener {
		DnsQuerySystem* _this;
		_(DnsQuerySystem* thiz) :
				_this(thiz) {
		}
		virtual void onFDToRead() THROWS {
			Utils::Log::d("onFDToRead");
			Utils::Looper::myLooper()->detachFD(_this->_selector);
			uint64_t count;
			ssize_t r = ::read(_this->_event, &count, sizeof(count));
			THROW_IF(r != sizeof(count) || count == 0,
					new Utils::Exception("FAILED to read eventfd"));
			_this->_listener->onDnsQueryResult(_this->_ip);
			delete _this;
		}
		virtual void onFDToWrite() {
		}
		virtual void onFDClosed() {
		}
		virtual void onFDError(Utils::Exception* e) THROWS {
			_this->_listener->onDnsQueryError(e);
		}
	} _eventListener;

	virtual ~DnsQuerySystem() {
		::pthread_spin_destroy(&_lock);
	}

	static void* _threadProc(void* thiz_) {
		DnsQuerySystem* thiz = (DnsQuerySystem*) thiz_;
		Utils::Log::d("--> gethostbyname '%s'", thiz->_hostname.sz());
		struct hostent* host = ::gethostbyname(thiz->_hostname);
		if (host && host->h_addr_list[0]) {
			thiz->_ip = ntohl(((struct in_addr*) host->h_addr_list[0])->s_addr);
			Utils::Log::d("%s <-- gethostbyname", IPv4::ntoa(thiz->_ip));
		} else {
			Utils::Log::d("null <-- gethostbyname");
		}
		if (::pthread_spin_trylock(&thiz->_lock) == 0) {
			Utils::Log::d("raise event");
			uint64_t count = 1;
			ssize_t r = ::write(thiz->_event, &count, sizeof(count));
			THROW_IF(r != sizeof(count),
					new Utils::Exception("FAILED to write eventfd"));
		} else {
			delete thiz;
		}
		return NULL;
	}

public:
	DnsQuerySystem(const char* hostname, DnsQueryListener* listener) :
			_hostname(hostname), _listener(listener), _tid(0), _ip(0), _eventListener(
					this) THROWS {
		int r = ::pthread_spin_init(&_lock, 0);
		THROW_IF(r != 0, new Utils::Exception("FAILED to init spin lock"));

		_event = ::eventfd(0, 0);
		THROW_IF(_event == -1,
				new Utils::Exception("FAILED to create eventfd, errno=%d", errno));
		_selector = Utils::Looper::myLooper()->attachFD(_event,
				&_eventListener);
		Utils::Looper::myLooper()->waitToRead(_selector);

		r = ::pthread_create(&_tid, NULL, _threadProc, this);
		THROW_IF(r != 0, new Utils::Exception("FAILED to create thread"));
		::pthread_detach(_tid);
		THROW_IF(r != 0, new Utils::Exception("FAILED to detach thread"));
	}

	void cancel() THROWS {
		if (::pthread_spin_trylock(&_lock) == 0)
			Utils::Looper::myLooper()->detachFD(_selector);
	}
};

DnsQuery* DnsClientSystem::query(const char* hostname,
		DnsQueryListener* listener) THROWS {
	return new DnsQuerySystem(hostname, listener);
}

}
