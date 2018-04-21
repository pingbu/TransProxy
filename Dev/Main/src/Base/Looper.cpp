#define LOG_TAG "Looper"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include "Utils.h"
#include "Debug.h"
#include "Looper.h"

#define MAX_FD_COUNT  1000

namespace Utils {

struct LooperImpl: Looper {
	struct {
		pollfd _fds[MAX_FD_COUNT];
		int _prev[MAX_FD_COUNT], _next[MAX_FD_COUNT], _first, _last, _free;
		FDListener* _listeners[MAX_FD_COUNT];

		void init() {
			for (int i = 0; i < MAX_FD_COUNT; ++i) {
				_fds[i].fd = -1;
				_next[i] = i + 1;
			}
			_next[MAX_FD_COUNT - 1] = _first = _last = -1;
			_free = 0;
		}

		int attach(int fd, FDListener* listener) THROWS {
			ASSERT(fd >= 0 && listener);
			int index = _free;
			THROW_IF(index < 0,
					new Utils::Exception("Looper fd buffer overflow!!!"));
			_free = _next[_free];
			if (_first < 0)
				_first = index;
			else
				_next[_last] = index;
			_prev[index] = _last;
			_next[index] = -1;
			_last = index;
			_fds[index].fd = fd;
			_fds[index].events = 0;
			_fds[index].revents = 0;
			_listeners[index] = listener;
			int flags = ::fcntl(fd, F_GETFL);
			THROW_IF(flags == -1, new Utils::Exception("F_GETFL failed."));
			if ((flags & O_NONBLOCK) == 0) {
				int r = ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
				THROW_IF(r == -1, new Utils::Exception("F_SETFL failed."));
			}
			Utils::Log::d("#%d <-- attachFD fd=%d", index, fd);
			return index;
		}

		void detach(int index) {
			ASSERT(_fds[index].fd >= 0 && _listeners[index]);
			Utils::Log::d("detach #%d", index);
			_fds[index].fd = -1;
			_fds[index].events = 0;
			_fds[index].revents = 0;
			_listeners[index] = NULL;
			if (_prev[index] < 0)
				_first = _next[index];
			else
				_next[_prev[index]] = _next[index];
			if (_next[index] < 0)
				_last = _prev[index];
			else
				_prev[_next[index]] = _prev[index];
			_next[index] = _free;
			_free = index;
		}

		void waitToRead(int index) {
			Utils::Log::d("#%d waitToRead", index);
			_fds[index].events = POLLIN | POLLHUP | POLLRDHUP | POLLERR;
		}

		void waitToWrite(int index) {
			Utils::Log::d("#%d waitToWrite", index);
			_fds[index].events = POLLOUT | POLLHUP | POLLRDHUP | POLLERR;
		}

		void loopOnce() THROWS {
			Utils::Log::v("--> poll");
			int eventCount = ::poll(_fds, MAX_FD_COUNT, -1);
			THROW_IF(eventCount < 0,
					new Utils::Exception("Looper poll FAILED, errno=%u!", errno));
			Utils::Log::v("<-- poll eventCount=%d", eventCount);
			for (int i = 0; eventCount > 0 && i < MAX_FD_COUNT; ++i)
				if (_fds[i].fd >= 0 && _fds[i].revents) {
					ASSERT(_listeners[i]);
					if ((_fds[i].revents & POLLIN)) {
						_fds[i].events &= ~POLLIN;
						Utils::Log::d("#%d onToRead", i);
						_listeners[i]->onFDToRead();
					} else if ((_fds[i].revents & POLLOUT)) {
						_fds[i].events &= ~POLLOUT;
						Utils::Log::d("#%d onToWrite", i);
						_listeners[i]->onFDToWrite();
					} else if ((_fds[i].revents & (POLLHUP | POLLRDHUP))) {
						_fds[i].events = 0;
						Utils::Log::d("#%d onClosed", i);
						_listeners[i]->onFDClosed();
					} else if ((_fds[i].revents & POLLERR)) {
						_fds[i].events = 0;
						Utils::Log::d("#%d onError", i);
						Utils::Exception* e = new Utils::Exception(
								"poll #%d Error", i);
						e->setTrace(__FUNCTION__, __FILE__, __LINE__);
						_listeners[i]->onFDError(e);
					}
					--eventCount;
				}
		}
	} FDs;

	LooperImpl() {
		FDs.init();
	}

	void loopOnce() THROWS {
		FDs.loopOnce();
	}
};

static LooperImpl* __fromLooper(Looper* looper) {
	THROW_IF(looper == NULL,
			new Utils::Exception("Looper called before preparing!!!"));
	return (LooperImpl*) looper;
}

static Utils::ThreadLocal<LooperImpl> _looper;

void Looper::prepare() THROWS {
	LooperImpl* looper = _looper.get();
	THROW_IF(looper != NULL, new Utils::Exception("Looper prepared already!!!"));
	_looper.set(new LooperImpl());
}

void Looper::loopOnce() THROWS {
	__fromLooper(myLooper())->loopOnce();
}

void Looper::loop() THROWS {
	for (LooperImpl* impl = __fromLooper(myLooper());;)
		impl->loopOnce();
}

Looper* Looper::myLooper() {
	return _looper.get();
}

int Looper::attachFD(int fd, FDListener* listener) THROWS {
	return __fromLooper(this)->FDs.attach(fd, listener);
}

void Looper::detachFD(int index) {
	__fromLooper(this)->FDs.detach(index);
}

void Looper::waitToRead(int index) {
	__fromLooper(this)->FDs.waitToRead(index);
}

void Looper::waitToWrite(int index) {
	__fromLooper(this)->FDs.waitToWrite(index);
}

}
