#include <errno.h>
#include <sys/timerfd.h>
#include "Debug.h"
#include "Log.h"
#include "Looper.h"

#pragma once

namespace Utils {

struct TimerListener {
	virtual ~TimerListener() {
	}
	virtual void onTimeout() THROWS = 0;
	virtual void onTimerError(Exception* e) THROWS = 0;
};

class Timer {
	String _name;
	TimerListener* _listener;
	int _fd, _selector;

	struct _: FDListener {
		Timer* _this;
		_(Timer* thiz) :
				_this(thiz) {
		}
		void onFDToRead() THROWS {
			uint64_t count;
			ssize_t r = ::read(_this->_fd, &count, sizeof(count));
			if (r != sizeof(count))
				Log::w("FAILED to read timer, errno=%d", errno);
			Log::d("onTimeout '%s'", _this->_name.sz());
			_this->_listener->onTimeout();
		}
		void onFDToWrite() THROWS {
		}
		void onFDClosed() THROWS {
		}
		void onFDError(Exception* e) THROWS {
			_this->_listener->onTimerError(e);
		}
	} _fdListener;

	void _init(const char* name, TimerListener* listener) THROWS {
		_name = name;
		_listener = listener;
		_fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
		THROW_IF(_fd == -1,
				new Exception("FAILED to create timer, errno=%d", errno));
		_selector = -1;
	}

public:
	Timer(const char* name, TimerListener* listener) :
			_fdListener(this) THROWS {
		_init(name, listener);
	}
	virtual ~Timer() {
		if (_selector > 0)
			Looper::myLooper()->detachFD(_selector);
	}

	void setTimeout(int timeout) {
		struct itimerspec spec = { 0 };
		if (timeout == 0) {
			spec.it_value.tv_nsec = 1;
		} else {
			spec.it_value.tv_sec = timeout / 1000;
			spec.it_value.tv_nsec = (timeout % 1000) * 1000000L;
		}
		int r = ::timerfd_settime(_fd, 0, &spec, NULL);
		THROW_IF(r != 0, new Exception("FAILED to set timer, errno=%d", errno));
		Log::d("setTimeout '%s', %ums", _name.sz(), timeout);
		if (_selector < 0)
			_selector = Looper::myLooper()->attachFD(_fd, &_fdListener);
		Looper::myLooper()->waitToRead(_selector);
	}
	void clearTimeout() {
		if (_selector > 0) {
			struct itimerspec spec = { 0 };
			int r = ::timerfd_settime(_fd, 0, &spec, NULL);
			THROW_IF(r != 0,
					new Exception("FAILED to clear timer, errno=%d", errno));
		}
		Log::d("clearTimeout '%s'", _name.sz());
	}
};

}
