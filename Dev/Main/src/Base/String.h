#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#pragma once

namespace Utils {

class String {
	char* _s;
	void _alloc(const char* s) {
		if (s == NULL) {
			_s = NULL;
		} else {
			_s = new char[::strlen(s) + 1];
			::strcpy(_s, s);
		}
	}
	void _alloc(const char* s, int l) {
		_s = new char[l + 1];
		::memcpy(_s, s, l);
		_s[l] = '\0';
	}
	void _free() {
		if (_s) {
			delete[] _s;
			_s = NULL;
		}
	}
public:
	String() {
		_s = NULL;
	}
	String(const char* s) {
		_alloc(s);
	}
	String(const char* s, int l) {
		_alloc(s, l);
	}
	String(const String& s) {
		_alloc(s);
	}
	~String() {
		_free();
	}
	String& operator=(const String& s) {
		_free();
		_alloc(s);
		return *this;
	}
	String& operator=(const char* s) {
		_free();
		_alloc(s);
		return *this;
	}
	String& operator+=(char c) {
		int l = ::strlen(_s);
		char* t = new char[l + 2];
		::strcpy(t, _s);
		t[l] = c;
		t[l + 1] = '\0';
		_free();
		_s = t;
		return *this;
	}
	String& operator+=(const char* s) {
		char* t = new char[::strlen(_s) + ::strlen(s) + 1];
		::strcpy(t, _s);
		::strcat(t, s);
		_free();
		_s = t;
		return *this;
	}
	String operator+(char c) const {
		String t;
		int l = ::strlen(_s);
		t._s = new char[l + 2];
		::strcpy(t._s, _s);
		t._s[l] = c;
		t._s[l + 1] = '\0';
		return t;
	}
	String operator+(const char* s) const {
		String t;
		t._s = new char[::strlen(_s) + ::strlen(s) + 1];
		::strcpy(t._s, _s);
		::strcat(t._s, s);
		return t;
	}
	operator bool() const {
		return _s != NULL;
	}
	bool operator!() const {
		return _s == NULL;
	}
	String& toString() {
		return *this;
	}
	const String& toString() const {
		return *this;
	}
	operator const char*() const {
		return _s;
	}
	const char* sz() const {
		return _s;
	}
	size_t length() const {
		return ::strlen(_s);
	}
	char operator[](int index) const {
		return _s[index];
	}
	bool operator<(const char* s) const {
		return ::strcmp(_s, s) < 0;
	}
	bool operator<=(const char* s) const {
		return ::strcmp(_s, s) <= 0;
	}
	bool operator==(const char* s) const {
		return ::strcmp(_s, s) == 0;
	}
	bool operator>=(const char* s) const {
		return ::strcmp(_s, s) >= 0;
	}
	bool operator>(const char* s) const {
		return ::strcmp(_s, s) > 0;
	}
	bool operator!=(const char* s) const {
		return ::strcmp(_s, s) != 0;
	}
	static String format(const char* fmt, ...) {
		char s[1024];
		va_list ap;
		va_start(ap, fmt);
		::vsprintf(s, fmt, ap);
		va_end(ap);
		return String(s);
	}
	static String formatV(const char* fmt, va_list ap) {
		char s[1024];
		::vsprintf(s, fmt, ap);
		return String(s);
	}
	static String repeat(const char* s, int n) {
		String v;
		int m = ::strlen(s);
		int l = m * n;
		v._s = new char[l + 1];
		for (int i = 0; i < l; i += m)
			::strcpy(v._s + i, s);
		return v;
	}

	bool startsWith(const char* prefix) const {
		int l = ::strlen(_s);
		int m = ::strlen(prefix);
		if (l < m)
			return false;
		return ::strncmp(_s, prefix, m) == 0;
	}

	bool endsWith(const char* postfix) const {
		int l = ::strlen(_s);
		int m = ::strlen(postfix);
		if (l < m)
			return false;
		return ::strcmp(_s + l - m, postfix) == 0;
	}

	bool contains(const char* ss) const {
		return ::strstr(_s, ss) != NULL;
	}

	int indexOf(const char* ss) const {
		char* p = ::strstr(_s, ss);
		if (p == NULL)
			return -1;
		return p - _s;
	}

	int indexOf(char c) const {
		char* p = ::strchr(_s, c);
		if (p == NULL)
			return -1;
		return p - _s;
	}

	String substring(int a) const {
		return String(_s + a);
	}

	String substring(int a, int b) const {
		return String(_s + a, b);
	}
};

class StringBuilder {
	char* _s;
	size_t _length, _limit;
	void _extendFor(size_t l) {
		size_t limit = _limit;
		while (limit < l)
			limit <<= 1;
		char* s = new char[limit + 1];
		if (_s) {
			::memcpy(s, _s, _length);
			delete[] _s;
		}
		_limit = limit;
		_s = s;
	}
public:
	StringBuilder(size_t limit = 1024) {
		_limit = limit;
		_s = new char[limit + 1];
		_length = 0;
	}
	StringBuilder(const char* s) {
		_limit = 1024;
		_s = NULL;
		_length = ::strlen(s);
		_extendFor(_length);
		::memcpy(_s, s, _length);
	}
	StringBuilder(const String& s) {
		_limit = 1024;
		_s = NULL;
		_length = s.length();
		_extendFor(_length);
		::memcpy(_s, s.sz(), _length);
	}
	~StringBuilder() {
		delete[] _s;
	}
	void operator+=(char c) {
		size_t l = _length + 1;
		if (l > _limit)
			_extendFor(l);
		_s[_length] = c;
		_length = l;
	}
	void operator+=(const char* s) {
		size_t l1 = ::strlen(s);
		size_t l = _length + l1;
		if (l > _limit)
			_extendFor(l);
		::memcpy(_s + _length, s, l1);
		_length = l;
	}
	void operator+=(const String& s) {
		*this += s.sz();
	}
	void clear() {
		_length = 0;
		*_s = '\0';
	}
	size_t length() const {
		return _length;
	}
	bool isEmpty() const {
		return _length == 0;
	}
	operator const char*() const {
		_s[_length] = '\0';
		return _s;
	}
	const char* toString() const {
		_s[_length] = '\0';
		return _s;
	}
};

}
