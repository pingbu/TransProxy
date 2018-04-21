#define LOG_TAG  "HttpClient"

#include <ctype.h>
#include "Base/Utils.h"
#include "HttpClient.h"

namespace Net {

class _Request: public HttpClient::Request {
	enum State {
		STATE_PREPARING,
		STATE_RESOLVING,
		STATE_CONNECTING,
		STATE_SEND_HEADER,
		STATE_SEND_CONTENT,
		STATE_RECEIVE_HEADER,
		STATE_RECEIVE_CONTENT,
		STATE_RECEIVE_CHUNK_LENGTH,
		STATE_RECEIVE_CHUNK_DATA,
		STATE_RECEIVE_CHUNK_TAIL,
		STATE_COMPLETED,
		STATE_CLOSED
	};

	DnsClient* _dns;
	TcpClientFactory* _factory;
	HttpClient::Listener* _listener;
	Utils::String _method, _schema, _host, _pathAndQuery, _user, _password,
			_contentType, _header;
	IPv4::SockAddr _addr;
	uint8_t* _content;
	size_t _contentLength, _sent;
	State _state;
	TcpConnection* _conn;
	Utils::Timer _timer;

	struct _Response: HttpClient::Response {
		_Request* _this;
		int status;
		Utils::StringBuilder header;
		Utils::String message, contentType;
		bool chunked;
		size_t contentLength;
		_Response(_Request* thiz) :
				_this(thiz), status(200), message("OK"), chunked(false), contentLength(
						0) {
		}
		void _parseFirstLine(char* line) THROWS;
		void _parseLine(char* line) THROWS;
		void _parseHeader(char* header) THROWS;
		int getStatus() const {
			return status;
		}
		const char* getMessage() const {
			return message;
		}
		const char* getContentType() const {
			return contentType;
		}
	} _response;

	struct _DnsQueryListener: DnsQueryListener {
		_Request* _this;
		_DnsQueryListener(_Request* thiz) :
				_this(thiz) {
		}
		void onDnsQueryResult(uint32_t ip) THROWS {
			_this->_addr.ip = ip;
			_this->_doHttp();
		}
		void onDnsQueryError(Utils::Exception* e) THROWS {
			_this->_listener->onHttpClientError(e);
		}
	} _dnsQueryListener;

	struct _TcpConnectionListener: TcpConnectionListener {
		_Request* _this;
		_TcpConnectionListener(_Request* thiz) :
				_this(thiz) {
		}
		void onTcpConnected() THROWS {
			_this->_doHttp();
		}
		void onTcpDisconnected() THROWS {
			if (_this->_state != STATE_CLOSED) {
				_this->_state = STATE_CLOSED;
				_this->_timer.setTimeout(0);
				_this->_listener->onHttpClientError(
				new Utils::Exception("Disconnected by peer"));
			}
		}
		void onTcpToRecv() THROWS {
			_this->_doHttp();
		}
		void onTcpToSend() THROWS {
			_this->_doHttp();
		}
		void onTcpError(Utils::Exception* e) THROWS {
			_this->_listener->onHttpClientError(e);
		}
	} _tcpConnectionListener;

	struct _TimerListener: Utils::TimerListener {
		_Request* _this;
		_TimerListener(_Request* thiz) :
				_this(thiz) {
		}
		void onTimeout() THROWS {
			_this->_doHttp();
		}
		void onTimerError(Utils::Exception* e) THROWS {
			_this->_listener->onHttpClientError(e);
		}
	} _timerListener;

	void _doHttp() THROWS;

public:
	_Request(DnsClient* dns, TcpClientFactory* factory, const char* method,
			const char* url, HttpClient::Listener* listener) THROWS;
	virtual ~_Request() {
		Utils::Log::d("==> ~Request");
		if (_conn)
			_conn->close();
		Utils::Log::d("<== ~Request");
	}

	void send(void* data, size_t bytes, const char* type) THROWS;
	void close() THROWS {
		Utils::Log::d("==> close");
		if (_state != STATE_CLOSED) {
			_state = STATE_CLOSED;
			_timer.setTimeout(0);
		}
		Utils::Log::d("<== close");
	}

};

HttpClient::Request* HttpClient::openRequest(const char* method,
		const char* url, HttpClient::Listener* listener) THROWS {
	return new _Request(_dns, _factory, method, url, listener);
}

_Request::_Request(DnsClient* dns, TcpClientFactory* factory,
		const char* method, const char* url, HttpClient::Listener* listener) :
		_dns(dns), _factory(factory), _listener(listener), _method("GET"), _content(
		NULL), _contentLength(0), _sent(0), _state(STATE_PREPARING), _conn(
		NULL), _timer("HttpClient", &_timerListener), _response(this), _dnsQueryListener(
				this), _tcpConnectionListener(this), _timerListener(this) THROWS {
	_method = method;
	_pathAndQuery = url;
	int p = _pathAndQuery.indexOf("://");
	THROW_IF(p < 0, new Utils::Exception("Invalid URL"));
	_schema = _pathAndQuery.substring(0, p);
	THROW_IF(_schema != "http" && _schema != "https",
			new Utils::Exception("Unsupported schema '%s'", _schema.toString().sz()));
	_pathAndQuery = _pathAndQuery.substring(p + 3);
	p = _pathAndQuery.indexOf('/');
	if (p < 0) {
		_host = _pathAndQuery;
		_pathAndQuery = "/";
	} else {
		_host = _pathAndQuery.substring(0, p);
		_pathAndQuery = _pathAndQuery.substring(p);
	}
	p = _host.indexOf('@');
	if (p >= 0) {
		_user = _host.substring(0, p);
		_host = _host.substring(p + 1);
		p = _user.indexOf(':');
		if (p > 0) {
			_password = _user.substring(0, p);
			_user = _user.substring(p + 1);
		}
	}
	p = _host.indexOf(':');
	if (p >= 0) {
		_addr.port = (uint16_t) ::atoi(_host.sz() + p + 1);
		_host = _host.substring(0, p);
	}
}

void _Request::_Response::_parseFirstLine(char* line) THROWS {
	char* p1 = ::strchr(line, ' ');
	THROW_IF(p1 == NULL,
			new Utils::Exception("Invalid HTTP response header!!!"));
	++p1;

	char* p2 = ::strchr(p1, ' ');
	if (p2) {
		*p2++ = '\0';
		message = p2;
		Utils::Log::v("Parsed MESSAGE=%s", message.sz());
	}

	status = ::atoi(p1);
	Utils::Log::v("Parsed STATUS=%d", status);
}

void _Request::_Response::_parseLine(char* line) THROWS {
	char* p = ::strchr(line, ':');
	THROW_IF(p == NULL, new Utils::Exception("Invalid HTTP response header!!!"));
	*p++ = '\0';
	while (*p == ' ')
		++p;

	if (::strcasecmp(line, "Content-Type") == 0) {
		contentType = p;
		Utils::Log::v("Parsed Content-Type=%s", contentType.sz());
	} else if (::strcasecmp(line, "Content-Length") == 0) {
		contentLength = ::atoi(p);
		Utils::Log::v("Parsed Content-Length=%u", contentLength);
	} else if (::strcasecmp(line, "Transfer-Encoding") == 0) {
		Utils::Log::v("Parsed Transfer-Encoding=%s", p);
		if (::strcasecmp(p, "chunked") == 0)
			chunked = true;
	}
}

void _Request::_Response::_parseHeader(char* header) THROWS {
	char* begin = header;
	char* end = ::strstr(begin, "\r\n");
	ASSERT(end);
	THROW_IF(end == begin,
			new Utils::Exception("Invalid HTTP response header!!!"));
	*end = '\0';
	_parseFirstLine(begin);
	for (;;) {
		begin = end + 2;
		end = ::strstr(begin, "\r\n");
		ASSERT(end);
		if (end == begin)
			break;
		*end = '\0';
		_parseLine(begin);
	}
}

void _Request::send(void* data, size_t bytes, const char* type) THROWS {
	THROW_IF(_state != STATE_PREPARING, new Utils::Exception("Invalid state"));
	_contentType = type;
	if (data && bytes) {
		_contentLength = bytes;
		_content = new uint8_t[bytes + 1];
		::memcpy(_content, data, bytes);
		_content[bytes] = '\0';
	}
	_timer.setTimeout(0);
}

void _Request::_doHttp() THROWS {
	if (_state == STATE_PREPARING) {
		_state = STATE_RESOLVING;
		/*_dnsQuery =*/_dns->query(_host, &_dnsQueryListener);
	} else if (_state == STATE_RESOLVING) {
		_conn = _factory->tcp_open(&_tcpConnectionListener);
		IPv4::SockAddr addr = _addr;
		if (_schema.endsWith("s")) {
			_conn = new Net::SSLConnection(_conn);
			if (addr.port == 0)
				addr.port = 443;
		} else {
			if (addr.port == 0)
				addr.port = 80;
		}
		_state = STATE_CONNECTING;
		_conn->connect(addr);
	} else if (_state == STATE_CONNECTING) {
		Utils::StringBuilder s = _method;
		s += ' ';
		s += _pathAndQuery;
		s += " HTTP/1.1\r\nHost: ";
		s += _host;
		if (_addr.port != 0) {
			s += ':';
			s += Utils::String::format("%u", _addr.port);
		}
		if (_contentType) {
			s += "\r\nContent-Type: ";
			s += _contentType;
		}
		if (_content) {
			s += "\r\nContent-Length: ";
			s += Utils::String::format("%u", _contentLength);
		}
		s += "\r\n\r\n";
		_header = s.toString();
		_state = STATE_SEND_HEADER;
		_sent = 0;
		_timer.setTimeout(0);
	} else if (_state == STATE_SEND_HEADER) {
		_sent += _conn->send(_header.sz() + _sent, _header.length() - _sent);
		if (_sent < _header.length()) {
			_conn->waitToSend();
		} else if (_content) {
			_state = STATE_SEND_CONTENT;
			_sent = 0;
			_timer.setTimeout(0);
		} else {
			_state = STATE_RECEIVE_HEADER;
			_conn->waitToRecv();
		}
	} else if (_state == STATE_SEND_CONTENT) {
		_sent += _conn->send(_content + _sent, _contentLength - _sent);
		if (_sent < _contentLength) {
			_conn->waitToSend();
		} else {
			_state = STATE_RECEIVE_HEADER;
			_conn->waitToRecv();
		}
	} else if (_state == STATE_RECEIVE_HEADER) {
		for (char buf[1500];;) {
			size_t l = _conn->peek(buf, sizeof(buf) - 1);
			if (l == 0) {
				_conn->waitToRecv();
				break;
			}
			buf[l] = '\0';
			size_t l0 = _response.header.length();
			_response.header += buf;
			char* t = (char*) _response.header.toString();
			char* p = ::strstr(t + Utils::max(l0, 0u), "\r\n\r\n");
			if (p == NULL) {
				l = _response.header.length() - l0;
				Utils::Log::d("read response %u bytes", l);
				_conn->recv(buf, l);
			} else {
				p += 4;
				*p = '\0';
				size_t headerLength = p - t;
				l = headerLength - l0;
				Utils::Log::d("read response %u bytes", l);
				_conn->recv(buf, l);
				_response._parseHeader(t);
				if (_response.chunked || _response.contentLength > 0) {
					_state =
							_response.chunked ?
									STATE_RECEIVE_CHUNK_LENGTH :
									STATE_RECEIVE_CONTENT;
					_sent = 0;
				} else {
					_state = STATE_COMPLETED;
				}
				_timer.setTimeout(0);
				_listener->onResponseHeader(&_response);
				break;
			}
		}
	} else if (_state == STATE_RECEIVE_CHUNK_LENGTH) {
		char buf[1500];
		size_t l = _conn->peek(buf, sizeof(buf) - 1);
		if (l > 0) {
			buf[l] = '\0';
			char* p = ::strstr(buf, "\r\n");
			if (p) {
				*p = '\0';
				p += 2;
				l = ::strtol(buf, NULL, 16);
				_conn->recv(buf, p - buf);
				Utils::Log::d("read chunk length = %u", l);
				_response.contentLength = l;
				_state = STATE_RECEIVE_CHUNK_DATA;
				_sent = 0;
				_timer.setTimeout(0);
			}
		} else {
			_conn->waitToRecv();
		}
	} else if (_state == STATE_RECEIVE_CONTENT
			|| _state == STATE_RECEIVE_CHUNK_DATA) {
		size_t l = 0;
		for (char buf[1500];;) {
			if (_sent >= _response.contentLength)
				break;
			size_t r = sizeof(buf);
			r = Utils::min(r, _response.contentLength - _sent);
			r = _conn->recv(buf, r);
			if (r == 0)
				break;
			_listener->onResponseContent(buf, r);
			if (_state == STATE_CLOSED)
				goto EndRecv;
			_sent += r;
			l += r;
		}
		Utils::Log::d("read content %u bytes", l);
		if (_sent >= _response.contentLength) {
			if (_state == STATE_RECEIVE_CONTENT)
				_state = STATE_COMPLETED;
			else
				_state = STATE_RECEIVE_CHUNK_TAIL;
			_timer.setTimeout(0);
		} else {
			_conn->waitToRecv();
		}
		EndRecv: ;
	} else if (_state == STATE_RECEIVE_CHUNK_TAIL) {
		char buf[2];
		size_t l = _conn->peek(buf, 2);
		if (l == 2) {
			_conn->recv(buf, 2);
			if (_response.contentLength > 0) {
				_state = STATE_RECEIVE_CHUNK_LENGTH;
			} else {
				_state = STATE_COMPLETED;
			}
			_timer.setTimeout(0);
		} else {
			_conn->waitToRecv();
		}
	} else if (_state == STATE_COMPLETED) {
		_state = STATE_CLOSED;
		_timer.setTimeout(0);
		_listener->onResponseCompleted();
	} else if (_state == STATE_CLOSED) {
		delete this;
	}
}

}
