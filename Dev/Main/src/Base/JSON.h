#include "Debug.h"
#include "String.h"
#include "Map.h"
#include "List.h"

#pragma once

namespace Utils {

class JSONObject;
class JSONArray;

struct _JSONVariant {
	enum Type {
		TYPE_NULL,
		TYPE_OBJECT,
		TYPE_ARRAY,
		TYPE_STRING,
		TYPE_INT,
		TYPE_FLOAT,
		TYPE_BOOL
	} type;
	union {
		JSONObject* objectValue;
		JSONArray* arrayValue;
		char* stringValue;
		long long intValue;
		double floatValue;
		bool boolValue;
	};
	_JSONVariant() {
		//Log::d("%p <-- new _JSONVariant()", this);
		type = TYPE_NULL;
	}
	void operator=(JSONObject* value) {
		_free();
		if (value != NULL) {
			type = TYPE_OBJECT;
			objectValue = value;
			//Log::d("_JSONVariant[%p]=JSONObject[%p]", this, value);
		} else {
			//Log::d("_JSONVariant[%p]=null", this);
		}
	}
	void operator=(JSONArray* value) {
		_free();
		if (value != NULL) {
			type = TYPE_ARRAY;
			arrayValue = value;
			//Log::d("_JSONVariant[%p]=JSONArray[%p]", this, value);
		} else {
			//Log::d("_JSONVariant[%p]=null", this);
		}
	}
	void operator=(const char* value) {
		_free();
		if (value != NULL) {
			type = TYPE_STRING;
			stringValue = new char[::strlen(value) + 1];
			::strcpy(stringValue, value);
			//Log::d("_JSONVariant[%p]=\"%s\"", this, value);
		} else {
			//Log::d("_JSONVariant[%p]=null", this);
		}
	}
	void operator=(long long value) {
		_free();
		type = TYPE_INT;
		intValue = value;
		//Log::d("_JSONVariant[%p]=%d", this, (int) value);
	}
	void operator=(double value) {
		_free();
		type = TYPE_FLOAT;
		floatValue = value;
		//Log::d("_JSONVariant[%p]=%f", this, value);
	}
	void operator=(bool value) {
		_free();
		type = TYPE_BOOL;
		boolValue = value;
		//Log::d("_JSONVariant[%p]=%s", this, value ? "true" : "false");
	}
	bool isNull() const THROWS {
		return this == NULL || type == _JSONVariant::TYPE_NULL;
	}
	operator JSONObject*() const THROWS {
		if (type == _JSONVariant::TYPE_NULL)
			return NULL;
		THROW_IF(type != _JSONVariant::TYPE_OBJECT,
				new Exception("Not object type"));
		return objectValue;
	}
	operator JSONArray*() const THROWS {
		if (type == _JSONVariant::TYPE_NULL)
			return NULL;
		THROW_IF(type != _JSONVariant::TYPE_ARRAY,
				new Exception("Not array type"));
		return arrayValue;
	}
	operator const char*() const THROWS {
		if (type == _JSONVariant::TYPE_NULL)
			return NULL;
		THROW_IF(type != _JSONVariant::TYPE_STRING,
				new Exception("Not string type"));
		return stringValue;
	}
	operator long long() const THROWS {
		THROW_IF(type != _JSONVariant::TYPE_INT,
				new Exception("Not integer type"));
		return intValue;
	}
	operator double() const THROWS {
		THROW_IF(type != _JSONVariant::TYPE_FLOAT,
				new Exception("Not float type"));
		return floatValue;
	}
	operator bool() const THROWS {
		THROW_IF(type != _JSONVariant::TYPE_BOOL,
				new Exception("Not boolean type"));
		return boolValue;
	}
	~_JSONVariant() {
		//Log::d("==> %p->~_JSONVariant()", this);
		_free();
		//Log::d("<== %p->~_JSONVariant()", this);
	}

	String toString() const;
private:
	void _free();
};

struct _JSONObjectItem: _JSONVariant, MapItem<String> {
	_JSONObjectItem(const char* key) :
			_key(key) {
	}
	String getKey() const {
		return _key;
	}
	String getKeyString() const {
		return _key;
	}
private:
	String _key;
};

class JSONObject {
	friend class JSONParser;
	int _refCount;
	Map<String, _JSONObjectItem> _map;
	~JSONObject() {
	}
	void _parse(const char* json) THROWS;
public:
	JSONObject(const char* json = NULL) :
			_refCount(1) THROWS {
		if (json)
			_parse(json);
	}
	JSONObject* addRef() {
		++_refCount;
		return this;
	}
	void release() {
		if (--_refCount == 0)
			delete this;
	}
	void clear() {
		_map.clear();
	}
	void putNull(const char* key) {
		_map.remove(key);
	}
	void put(const char* key, JSONObject* value) {
		_map.remove(key);
		if (value != NULL) {
			_JSONObjectItem* item = new _JSONObjectItem(key);
			*(_JSONVariant*) item = value;
			_map.add(item);
		}
	}
	void put(const char* key, JSONArray* value) {
		_map.remove(key);
		if (value != NULL) {
			_JSONObjectItem* item = new _JSONObjectItem(key);
			*(_JSONVariant*) item = value;
			_map.add(item);
		}
	}
	void put(const char* key, const char* value) {
		_map.remove(key);
		if (value != NULL) {
			_JSONObjectItem* item = new _JSONObjectItem(key);
			*(_JSONVariant*) item = value;
			_map.add(item);
		}
	}
	void put(const char* key, int value) {
		_map.remove(key);
		_JSONObjectItem* item = new _JSONObjectItem(key);
		*(_JSONVariant*) item = (long long) value;
		_map.add(item);
	}
	void put(const char* key, long long value) {
		_map.remove(key);
		_JSONObjectItem* item = new _JSONObjectItem(key);
		*(_JSONVariant*) item = value;
		_map.add(item);
	}
	void put(const char* key, double value) {
		_map.remove(key);
		_JSONObjectItem* item = new _JSONObjectItem(key);
		*(_JSONVariant*) item = value;
		_map.add(item);
	}
	void put(const char* key, bool value) {
		_map.remove(key);
		_JSONObjectItem* item = new _JSONObjectItem(key);
		*(_JSONVariant*) item = value;
		_map.add(item);
	}
	bool isNull(const char* key) const THROWS {
		return _map.get(key)->isNull();
	}
	JSONObject* getJSONObject(const char* key) const THROWS {
		return *(_JSONVariant*) _map.get(key);
	}
	JSONArray* getJSONArray(const char* key) const THROWS {
		return *(_JSONVariant*) _map.get(key);
	}
	const char* getString(const char* key) const THROWS {
		return *(_JSONVariant*) _map.get(key);
	}
	long long getInt(const char* key) const THROWS {
		return *(_JSONVariant*) _map.get(key);
	}
	double getFloat(const char* key) const THROWS {
		return *(_JSONVariant*) _map.get(key);
	}
	bool getBool(const char* key) const THROWS {
		return *(_JSONVariant*) _map.get(key);
	}
	String toString() const;
};

struct _JSONArrayItem: _JSONVariant, ListItem {
};

class JSONArray {
	friend class JSONParser;
	int _refCount;
	List<_JSONArrayItem> _list;
	~JSONArray() {
	}
	_JSONVariant* _get(size_t index) const THROWS {
		THROW_IF(index >= _list.size(), new Exception("Index out of boundary"));
		_JSONArrayItem* item = _list.first();
		while (index--)
			item = _list.next(item);
		return item;
	}
	void _parse(const char* json) THROWS;
public:
	JSONArray(const char* json = NULL) :
			_refCount(1) {
		if (json)
			_parse(json);
	}
	JSONArray* addRef() {
		++_refCount;
		return this;
	}
	void release() {
		if (--_refCount == 0)
			delete this;
	}
	void clear() {
		_list.clear();
	}
	void put(JSONObject* value) {
		_JSONArrayItem* item = new _JSONArrayItem();
		*(_JSONVariant*) item = value;
		_list.insertTail(item);
	}
	void put(JSONArray* value) {
		_JSONArrayItem* item = new _JSONArrayItem();
		*(_JSONVariant*) item = value;
		_list.insertTail(item);
	}
	void put(const char* value) {
		_JSONArrayItem* item = new _JSONArrayItem();
		*(_JSONVariant*) item = value;
		_list.insertTail(item);
	}
	void put(long long value) {
		_JSONArrayItem* item = new _JSONArrayItem();
		*(_JSONVariant*) item = value;
		_list.insertTail(item);
	}
	void put(double value) {
		_JSONArrayItem* item = new _JSONArrayItem();
		*(_JSONVariant*) item = value;
		_list.insertTail(item);
	}
	void put(bool value) {
		_JSONArrayItem* item = new _JSONArrayItem();
		*(_JSONVariant*) item = value;
		_list.insertTail(item);
	}
	bool isNull(size_t index) const THROWS {
		return _get(index)->isNull();
	}
	JSONObject* getJSONObject(size_t index) const THROWS {
		return *_get(index);
	}
	JSONArray* getJSONArray(size_t index) const THROWS {
		return *_get(index);
	}
	const char* getString(size_t index) const THROWS {
		return *_get(index);
	}
	long long getInt(size_t index) const THROWS {
		return *_get(index);
	}
	double getFloat(size_t index) const THROWS {
		return *_get(index);
	}
	bool getBool(size_t index) const THROWS {
		return *_get(index);
	}
	String toString() const;
};

}
