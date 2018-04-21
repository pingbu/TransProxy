#define LOG_TAG "HttpServer"

#include <stdio.h>
#include "Base/Debug.h"
#include "Base/Utils.h"
#include "HttpServer.h"

namespace Net {

class HttpServerConnection: public TcpConnectionListener,
		HttpRequest,
		HttpResponse {
	enum {
		STATE_REQUEST_HEADER,
		STATE_REQUEST_CONTENT,
		STATE_RESPONSE_CACHE,
		STATE_RESPONSE_SENDING
	};

	TcpConnection* _conn;
	HttpServerListener* _listener;

	char _request[32768], _responseContent[32768];
	size_t _requestPosition, _requestHeaderLength, _requestContentLength,
			_requestLength, _responseContentLength;
	bool _http11;
	Utils::String _method, _path, _host, _requestContentType, _statusMsg,
			_responseContentType;
	Utils::Map<Utils::String, Utils::StringMapItem<Utils::String> > _queryStrings;
	int _statusCode, _state;

	void _reset() {
		_requestPosition = _requestHeaderLength = _requestContentLength =
				_requestLength = _responseContentLength = 0;
		_state = STATE_REQUEST_HEADER;
		_conn->waitToRecv();
	}

	void _parseQueryString(char* query) {
		char* p = ::strchr(query, '=');
		if (p)
			*p++ = '\0';
		_queryStrings.add(new Utils::StringMapItem<Utils::String>(query, p));
		Utils::Log::v("Parsed QueryString[\"%s\"]=\"%s\"", query, p);
	}

	void _parseQueryStrings(char* query) {
		for (char* a = query;;) {
			char* b = ::strchr(a, '&');
			if (b) {
				*b = '\0';
				_parseQueryString(a);
				a = b + 1;
			} else {
				_parseQueryString(a);
				break;
			}
		}
	}

	void _parseFirstLine(char* line) THROWS {
		char* p1 = ::strchr(line, ' ');
		THROW_IF(p1 == NULL,
				new Utils::Exception("Invalid HTTP request header!!!"));
		*p1++ = '\0';
		_method = line;
		Utils::Log::v("Parsed METHOD=%s", _method.sz());

		char* p2 = ::strchr(p1, ' ');
		THROW_IF(p2 == NULL,
				new Utils::Exception("Invalid HTTP request header!!!"));
		*p2++ = '\0';
		char* q = ::strchr(p1, '?');
		if (q) {
			*q++ = '\0';
			if (*q)
				_parseQueryStrings(q);
		}
		_path = p1;
		Utils::Log::v("Parsed PATH=%s", _path.sz());

		_http11 = ::strcasecmp(p2, "HTTP/1.1") == 0;
		Utils::Log::v("Parsed HTTP/1.1=%s", _http11 ? "true" : "false");
	}

	void _parseLine(char* line) THROWS {
		char* p = ::strchr(line, ':');
		THROW_IF(p == NULL,
				new Utils::Exception("Invalid HTTP request header!!!"));
		*p++ = '\0';
		while (*p > '\0' && *p <= ' ')
			++p;

		if (::strcasecmp(line, "Host") == 0) {
			_host = p;
			Utils::Log::v("Parsed Host=%s", _host.sz());
		} else if (::strcasecmp(line, "Content-Type") == 0) {
			_requestContentType = p;
			Utils::Log::v("Parsed Content-Type=%s", _requestContentType.sz());
		} else if (::strcasecmp(line, "Content-Length") == 0) {
			_requestContentLength = ::atoi(p);
			Utils::Log::v("Parsed Content-Length=%u", _requestContentLength);
		}
	}

	void _parseHeader(char* header) THROWS {
		_http11 = false;
		_host = "";
		_method = _path = _requestContentType = NULL;
		_queryStrings.clear();
		char* begin = header;
		char* end = ::strstr(begin, "\r\n");
		ASSERT(end);
		THROW_IF(end == begin,
				new Utils::Exception("Invalid HTTP request header!!!"));
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
		_state = STATE_REQUEST_CONTENT;
	}

	void _send(const void* data, size_t bytes) THROWS {
		for (;;) {
			size_t r = _conn->send(data, bytes);
			bytes -= r;
			if (bytes == 0)
				break;
			data = (uint8_t*) data + r;
			_conn->waitToSend();
			Utils::Looper::myLooper()->loopOnce();
		}
	}

	// HttpRequest

	IPv4::SockAddr getRemoteAddr() const {
		return _conn->getRemoteAddr();
	}

	const char* getMethod() const {
		return _method;
	}

	const char* getHost() const {
		return _host;
	}

	const char* getPath() const {
		return _path;
	}

	const char* getQueryString(const char* name) const {
		Utils::StringMapItem<Utils::String>* item = _queryStrings.get(name);
		return item ? item->getValue().sz() : NULL;
	}

	const char* getContentType() const {
		return _requestContentType;
	}

	const void* getContent() const {
		return _request + _requestHeaderLength;
	}

	size_t getContentLength() const {
		return _requestContentLength;
	}

	// HttpResponse

	void setStatus(int code, const char* msg) {
		if (_state != STATE_RESPONSE_CACHE)
			Utils::Log::w("setStatus after header sent");
		_statusCode = code;
		_statusMsg = msg;
	}

	void setContentType(const char* contentType) {
		if (_state != STATE_RESPONSE_CACHE)
			Utils::Log::w("setContentType after header sent");
		_responseContentType = contentType;
	}

	void write(const void* data, size_t bytes) {
		if (_state == STATE_RESPONSE_CACHE) {
			size_t totalBytes = _responseContentLength + bytes;
			if (totalBytes <= sizeof(_responseContent)) {
				::memcpy(_responseContent + _responseContentLength, data,
						bytes);
				_responseContentLength = totalBytes;
			} else {
				_flush(false);
			}
		}

		if (_state == STATE_RESPONSE_SENDING)
			_send(data, bytes);
	}

	void _sendHeader(bool end, size_t contentLength) {
		char* header = (char*) ::alloca(256);
		char* p = header;

		::sprintf(p, "HTTP/1.%u %d %s\r\n", _http11 ? 1 : 0, _statusCode,
				_statusMsg.sz());
		p = p + ::strlen(p);

		::sprintf(p, "Content-Type: %s\r\n", _responseContentType.sz());
		p = p + ::strlen(p);

		if (end) {
			::sprintf(p, "Content-Length: %u\r\n", contentLength);
			p = p + ::strlen(p);
		}

		if (_http11) {
			::sprintf(p, "Connection: %s\r\n", end ? "Keep-Alive" : "close");
			p = p + ::strlen(p);
		}

		::sprintf(p, "Cache-Control: no-cache\r\n");
		p = p + ::strlen(p);

		::sprintf(p, "Age: 0\r\n");
		p = p + ::strlen(p);

		::sprintf(p, "\r\n");
		p = p + ::strlen(p);

		_send(header, p - header);
		_state = STATE_RESPONSE_SENDING;
	}

	void writeFile(const char* path) {
		FILE* fp = ::fopen(path, "r" "b");
		if (fp == NULL) {
			setStatus(404, "Not Found");
		} else {
			setStatus(200, "OK");
			::fseek(fp, 0, SEEK_END);
			size_t l = ::ftell(fp);
			_sendHeader(true, l);
			::fseek(fp, 0, SEEK_SET);
			char buf[1024];
			while (l > sizeof(buf)) {
				::fread(buf, sizeof(buf), 1, fp);
				_send(buf, sizeof(buf));
				l -= sizeof(buf);
			}
			::fread(buf, l, 1, fp);
			_send(buf, l);
			::fclose(fp);
			_reset();
		}
	}

	void _flush(bool end) {
		_sendHeader(end, _responseContentLength);
		if (_responseContentLength > 0)
			_send(_responseContent, _responseContentLength);
		if (end)
			_reset();
	}

	void flush() {
		_flush(false);
	}

	void end() {
		if (_state == STATE_RESPONSE_CACHE) {
			_flush(true);
		} else if (_state == STATE_RESPONSE_SENDING) {
			Utils::Log::d("Disconnected by server.");
			delete this;
		}
	}

public:
	HttpServerConnection(TcpConnection* conn, HttpServerListener* listener) :
			_conn(conn), _listener(listener) {
		_reset();
	}

	~HttpServerConnection() {
		_conn->close();
	}

	void onTcpConnected() THROWS {
	}

	void onTcpDisconnected() THROWS {
		Utils::Log::d("Disconnected by client.");
		delete this;
	}

	void onTcpToRecv() THROWS {
		THROW_IF(
				_state != STATE_REQUEST_HEADER
						&& _state != STATE_REQUEST_CONTENT,
				new Utils::Exception( "received HTTP request data while response"));

		size_t bytes = _conn->recv(_request + _requestPosition,
				sizeof(_request) - _requestPosition);
		size_t totalBytes = _requestPosition + bytes;

		if (_state == STATE_REQUEST_HEADER && totalBytes >= 4) {
			_request[totalBytes] = '\0';
			char* p = ::strstr(
					_request
							+ (_requestPosition < 4 ? 0 : _requestPosition - 3),
					"\r\n\r\n");
			if (p) {
				_parseHeader(_request);
				_requestHeaderLength = p + 4 - _request;
				_requestLength = _requestHeaderLength + _requestContentLength;
				_state = STATE_REQUEST_CONTENT;
			} else if (totalBytes >= sizeof(_request)) {
				Utils::Log::e("HTTP request overflow, connection discard!!!");
				delete this;
				return;
			}
		}
		_requestPosition = totalBytes;

		if (_state == STATE_REQUEST_CONTENT
				&& _requestPosition == _requestLength) {
			_request[_requestHeaderLength + _requestContentLength] = '\0';
			_state = STATE_RESPONSE_CACHE;
			setStatus(404, "Not Found");
			setContentType("text/html; charset=UTF-8");
			Utils::Log::d("%s http://%s%s", _method.sz(), _host.sz(),
					_path.sz());
			_listener->onHttpRequest(*this, *this);
			Utils::Log::i("%d %s <-- %s http://%s%s", _statusCode,
					_statusMsg.sz(), _method.sz(), _host.sz(), _path.sz());
			end();
		}

		_conn->waitToRecv();
	}

	void onTcpToSend() THROWS {
	}

	void onTcpError(Utils::Exception* e) THROWS {
		e->print();
		delete this;
	}
};

TcpConnectionListener* HttpServer::onTcpServerConnected(TcpConnection* conn)
		THROWS {
	return new HttpServerConnection(conn, _listener);
}

}
