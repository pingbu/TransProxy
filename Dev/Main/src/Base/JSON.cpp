//#define LOG_TAG "JSON"

#include "Debug.h"
#include "Log.h"
#include "JSON.h"

namespace Utils {

static char _escapeCodes[] = "nrtvabf'\"\\?";
static char _escapeChars[] = "\n\r\t\v\a\b\f'\"\\?";

static char _octChars[] = "01234567";
static char _hexChars[] = "0123456789ABCDEFabcdef";

static String __jsonEncodeString(const char* s) {
	StringBuilder sb;
	sb += '\"';
	char c;
	while ((c = *s++) != '\0') {
		char* p = ::strchr(_escapeChars, c);
		if (p) {
			sb += '\\';
			sb += _escapeCodes[p - _escapeChars];
		} else if (c > '\0' && c <= '\x7F') {
			sb += c;
		} else {
			char t[16];
			::sprintf(t, "\\x%02x", c);
			sb += t;
		}
	}
	sb += '\"';
	return sb.toString();
}

String _JSONVariant::toString() const {
	if (type == TYPE_OBJECT)
		return objectValue->toString();
	else if (type == TYPE_ARRAY)
		return arrayValue->toString();
	else if (type == TYPE_STRING)
		return __jsonEncodeString(stringValue);
	else if (type == TYPE_INT)
		return String::format("%d", intValue);
	else if (type == TYPE_FLOAT)
		return String::format("%f", floatValue);
	else if (type == TYPE_BOOL)
		return boolValue ? "true" : "false";
	return "null";
}

void _JSONVariant::_free() {
	if (type == TYPE_OBJECT)
		objectValue->release();
	else if (type == TYPE_ARRAY)
		arrayValue->release();
	else if (type == TYPE_STRING)
		delete[] stringValue;
	type = TYPE_NULL;
}

String JSONObject::toString() const {
	StringBuilder r;
	r += '{';
	bool first = true;
	for (_JSONObjectItem* item = _map.min(); item; item = _map.bigger(item)) {
		if (first)
			first = false;
		else
			r += ',';
		r += '\"';
		r += item->getKey().sz();
		r += "\":";
		r += item->toString().sz();
	}
	r += '}';
	return r.toString();
}

String JSONArray::toString() const {
	StringBuilder r;
	r += "[";
	for (size_t i = 0; i < _list.size(); ++i) {
		if (i)
			r += ',';
		r += _get(i)->toString().sz();
	}
	r += ']';
	return r.toString();
}

class JSONParser {
	static inline bool _isspace(char c) {
		return c > '\0' && c <= ' ';
	}

	static String _parseString(const char*& s) THROWS {
		StringBuilder sb;
		for (;;) {
			char c = *s++;
			THROW_IF(c == '\0', new Exception("Invalid JSON string"));
			if (c == '\"') {
				break;
			} else if (c == '\\') {
				c = *s++;
				char* p = ::strchr(_escapeCodes, c);
				if (p) {
					sb += _escapeChars[p - _escapeCodes];
				} else if (c == '0') {
					sb += '\0';
				} else if (::strchr(_octChars, c)) {
					int l;
					if (::strchr(_octChars, *s))
						if (::strchr(_octChars, s[1]))
							l = 3;
						else
							l = 2;
					else
						l = 1;
					char t[4];
					::memcpy(t, s - 1, l);
					t[l] = '\0';
					sb += (char) ::strtol(t, NULL, 8);
					s += l - 1;
				} else if (c == 'x') {
					THROW_IF(!::strchr(_hexChars, *s),
							new Exception("Invalid JSON string"));
					int l;
					if (::strchr(_hexChars, s[1]))
						l = 2;
					else
						l = 1;
					char t[3];
					::memcpy(t, s - 1, l);
					t[l] = '\0';
					sb += (char) ::strtol(t, NULL, 16);
					s += l - 1;
				} else {
					sb += c;
				}
			} else {
				sb += c;
			}
		}
		return sb.toString();
	}

	static void _parseValue(_JSONVariant* item, const char*& s) THROWS {
		_skipSpaces(s);
		if (*s == '{') {
			++s;
			JSONObject* object = new JSONObject();
			_parseJSONObject(object, s);
			*item = object;
			Log::d("JSONObject <== _parseValue");
			return;
		}
		if (*s == '[') {
			++s;
			JSONArray* array = new JSONArray();
			_parseJSONArray(array, s);
			*item = array;
			Log::d("JSONObject <== _parseValue");
			return;
		}
		if (*s == '\"') {
			++s;
			String value = _parseString(s);
			*item = value.sz();
			Log::d("\"%s\" <== _parseValue", value.sz());
			return;
		}
		const char* p = s++;
		while (*s != ',' && *s != '}' && *s != '\0' && !_isspace(*s))
			++s;
		size_t length = s - p;
		char value[length + 1];
		::memcpy(value, p, length);
		value[length] = '\0';
		Log::d("value=\"%s\"", value);
		if (::strcmp(value, "null") == 0) {
			Log::d("null <== _parseValue");
			return;
		}
		if (::strcmp(value, "true") == 0) {
			*item = true;
			Log::d("true <== _parseValue");
			return;
		}
		if (::strcmp(value, "false") == 0) {
			*item = false;
			Log::d("false <== _parseValue");
			return;
		}
		char* end;
		long long intValue = ::strtoll(value, &end, 0);
		if (*end == '\0') {
			*item = intValue;
			Log::d("%d <== _parseValue", (int) intValue);
			return;
		}
		double floatValue = ::strtod(value, &end);
		THROW_IF(*end != '\0', new Exception("Invalid JSON string"));
		*item = floatValue;
		Log::d("%f <== _parseValue", floatValue);
	}

public:
	static inline void _skipSpaces(const char*& s) {
		while (_isspace(*s))
			++s;
	}

	static void _parseJSONObject(JSONObject* object, const char*& s) THROWS {
		for (;;) {
			_skipSpaces(s);
			char c = *s++;
			if (c == '}')
				return;
			THROW_IF(c != '\"', new Exception("Invalid JSON string"));

			String key = _parseString(s);
			Log::d("key=\"%s\"", key.sz());
			THROW_IF(object->_map.get(key),
					new Exception("Invalid JSON string"));

			_skipSpaces(s);
			THROW_IF(*s++ != ':', new Exception("Invalid JSON string"));

			_JSONObjectItem* item = new _JSONObjectItem(key);
			_parseValue(item, s);
			object->_map.add(item);

			_skipSpaces(s);
			c = *s++;
			if (c == '}')
				return;
			THROW_IF(c != ',', new Exception("Invalid JSON string"));
		}
	}

	static void _parseJSONArray(JSONArray* array, const char*& s) THROWS {
		for (;;) {
			_skipSpaces(s);
			if (*s == ']') {
				++s;
				return;
			}
			_JSONArrayItem* item = new _JSONArrayItem();
			_parseValue(item, s);
			array->_list.insertTail(item);

			_skipSpaces(s);
			char c = *s++;
			if (c == ']')
				return;
			THROW_IF(c != ',', new Exception("Invalid JSON string"));
		}
	}
};

void JSONObject::_parse(const char* s) THROWS {
	Log::v(s);
	Log::d("--> _skipSpaces");
	JSONParser::_skipSpaces(s);
	Log::d("<-- _skipSpaces");
	THROW_IF(*s++ != '{', new Exception("Invalid JSON string"));
	JSONParser::_parseJSONObject(this, s);
	JSONParser::_skipSpaces(s);
	THROW_IF(*s != '\0', new Exception("Invalid JSON string"));
}

void JSONArray::_parse(const char* s) THROWS {
	JSONParser::_skipSpaces(s);
	THROW_IF(*s++ != '[', new Exception("Invalid JSON string"));
	JSONParser::_parseJSONArray(this, s);
	JSONParser::_skipSpaces(s);
	THROW_IF(*s != '\0', new Exception("Invalid JSON string"));
}

}
