#include <stddef.h>
#include <stdlib.h>
#include "Debug.h"
#include "String.h"
#include "Math.h"
#include "Log.h"

#pragma once

namespace Utils {

class _MapItem {
	_MapItem *_p, *_l, *_r;
	int _height;
public:
	virtual ~_MapItem() {
	}
	_MapItem*& p() THROWS {
		ASSERT(this);
		return _p;
	}
	_MapItem*& l() THROWS {
		ASSERT(this);
		return _l;
	}
	_MapItem*& r() THROWS {
		ASSERT(this);
		return _r;
	}
	_MapItem* p() const THROWS {
		ASSERT(this);
		return _p;
	}
	_MapItem* l() const THROWS {
		ASSERT(this);
		return _l;
	}
	_MapItem* r() const THROWS {
		ASSERT(this);
		return _r;
	}
	int height() const THROWS {
		ASSERT(this);
		return _height;
	}
	_MapItem* p(_MapItem* p) THROWS {
		ASSERT(this);
		return (_p = p);
	}
	_MapItem* l(_MapItem* l) THROWS {
		ASSERT(this);
		return (_l = l);
	}
	_MapItem* r(_MapItem* r) THROWS {
		ASSERT(this);
		return (_r = r);
	}
	int height(int height) THROWS {
		ASSERT(this);
		return (_height = height);
	}
};

template<typename Key>
struct MapItem: _MapItem {
	virtual ~MapItem() {
	}
	virtual Key getKey() const = 0;
	virtual String getKeyString() const = 0;
};

template<typename Key, class T>
class MapItemPtr: public MapItem<Key> {
	T* _p;
public:
	MapItemPtr(T* p) {
		_p = p;
	}
	operator T*() const {
		return this == NULL ? NULL : _p;
	}
	T* operator->() const {
		return this == NULL ? NULL : _p;
	}
};

template<typename T, const char* fmt>
class ValueSetItem: public MapItem<T> {
	T _value;
public:
	ValueSetItem(T value) {
		_value = value;
	}
	T getKey() const {
		ASSERT(this);
		return _value;
	}
	String getKeyString() const {
		ASSERT(this);
		return String::format(fmt, _value);
	}
	const T& getValue() const {
		ASSERT(this);
		return _value;
	}
};

template<typename T>
class ObjectSetItem: public MapItem<T> {
	T _value;
public:
	ObjectSetItem(const T& value) {
		_value = value;
	}
	T getKey() const {
		ASSERT(this);
		return _value;
	}
	String getKeyString() const {
		ASSERT(this);
		return _value.toString();
	}
	const T& getValue() const {
		ASSERT(this);
		return _value;
	}
};

class StringSetItem: public MapItem<String> {
	String _value;
public:
	StringSetItem(const char* value) {
		_value = value;
	}
	String getKey() const {
		ASSERT(this);
		return _value;
	}
	String getKeyString() const {
		ASSERT(this);
		return _value;
	}
	const char* getValue() const {
		return this == NULL ? NULL : _value.sz();
	}
};

template<typename Key, const char* fmt, typename T>
class ValueMapItem: public MapItem<T> {
	Key _key;
	T _value;
public:
	ValueMapItem(Key key, const T& value) :
			_key(key), _value(value) {
	}
	Key getKey() const {
		return _key;
	}
	String getKeyString() const {
		return String::format(fmt, _key);
	}
	const T& getValue() const {
		return _value;
	}
};

template<typename Key, typename T>
class ObjectMapItem: public MapItem<T> {
	Key _key;
	T _value;
public:
	ObjectMapItem(const Key& key, const T& value) :
			_key(key), _value(value) {
	}
	Key getKey() const {
		return _key;
	}
	String getKeyString() const {
		return _key.toString();
	}
	const T& getValue() const {
		return _value;
	}
};

template<typename T>
class StringMapItem: public MapItem<T> {
	String _key;
	T _value;
public:
	StringMapItem(const char* key, const T& value) :
			_key(key), _value(value) {
	}
	String getKey() const {
		return _key;
	}
	String getKeyString() const {
		return _key;
	}
	const T& getValue() const {
		return _value;
	}
};

class _Map {
	static void __free(_MapItem* root);
	void __adjust(_MapItem*& root);
	void __remove(_MapItem*& node);
	_MapItem* __removeMin(_MapItem*& root);
	_MapItem* __removeMax(_MapItem*& root);
	void __checkParent(const _MapItem* parent, const _MapItem* node) const;
	bool __checkMinMax(const _MapItem* root, const _MapItem*& minNode,
			const _MapItem*& maxNode) const;
protected:
	String _name;
	_MapItem* _root;
	size_t _size;
	_Map(const char* name) :
			_name(name), _root(NULL), _size(0) {
	}
	virtual ~_Map() {
		clear();
	}
	virtual String __getKeyString(const _MapItem* item) const = 0;
	virtual int __compare(const _MapItem* a, const _MapItem* b) const = 0;
	bool __insert(_MapItem* parent, _MapItem*& root, _MapItem* item);
	bool __remove(_MapItem*& root, _MapItem* item);
	void __checkOrder() const;
	static int __count(const _MapItem* node) {
		return node == NULL ? 0 : __count(node->l()) + 1 + __count(node->r());
	}
	void __dump(const _MapItem* item, const char* p, const char* pl,
			const char* pr) const;
	void dump() const {
		if (!_name)
			return;
		if (_root) {
			String subPrefix = String::repeat(" ", _name.length() + 2);
			__dump(_root, String::format("%s --", _name.sz()), subPrefix.sz(),
					subPrefix.sz());
		} else {
			Log::v("%s-- (null)", _name.sz());
		}
	}
public:
	bool isEmpty() const {
		return _root == NULL;
	}
	void clear() {
		__free(_root);
		_root = NULL;
		_size = 0;
	}
	size_t size() const {
		return _size;
	}
};

template<typename Key, class T>
class Map: public _Map {
	String __getKeyString(const _MapItem* item) const {
		return item == NULL ? "(null)" : ((const T*) item)->getKeyString();
	}
	int __compare(const _MapItem* a, const _MapItem* b) const {
		return compare(((const T*) a)->getKey(), ((const T*) b)->getKey());
	}

public:
	Map(const char* name = NULL) :
			_Map(name) {
	}

	bool add(T* item) {
		//if (_name)
		//	Log::d("%s <--add-- %s", _name.sz(), __getKeyString(item).sz());
		bool r = __insert(NULL, _root, item);
		if (_name && !r)
			Log::w("Node in tree ready, ignore add");
		__checkOrder();
		return r;
	}

	bool remove(T* item) {
		int c0 = __count(_root);
		if (c0 == 0) {
			Log::e("Map[%s] remove node '%s' strange from '%s'! c0=0",
					_name.sz(), __getKeyString(item).sz(),
					__getKeyString(_root).sz());
			dump();
			::abort();
		}
		//if (_name)
		//	Log::d("%s --remove--> %s", _name.sz(), __getKeyString(item).sz());
		bool r = __remove(_root, item);
		if (_name && !r)
			Log::w("Node not in tree, ignore remove");
		__checkOrder();
		int c1 = __count(_root);
		if (c0 - c1 != 1) {
			Log::e("Map[%s] remove node '%s' from '%s' ERROR!!! c0=%d, c1=%d",
					_name.sz(), __getKeyString(item).sz(),
					__getKeyString(_root).sz(), c0, c1);
			dump();
			::abort();
		}
		//if (_name)
		//	Log::d("%s <== remove %s", r ? "true" : "false",
		//			__getKeyString(item).sz());
		if (!r)
			::abort();
		return r;
	}

	void remove(Key key) {
		T* item = get(key);
		if (item) {
			remove(item);
			delete item;
		}
	}

	T* min() const {
		T* node = (T*) _root;
		if (node)
			while (node->l())
				node = (T*) node->l();
		return node;
	}

	T* max() const {
		T* node = (T*) _root;
		if (node)
			while (node->r())
				node = (T*) node->r();
		return node;
	}

	T* smaller(T* item) const {
		T* node = item->l();
		if (node) {
			while (node->r())
				node = (T*) node->r();
			return node;
		}
		node = item;
		T* parent = node->p();
		while (parent && parent->l() == node) {
			node = parent;
			parent = node->p();
		}
		return parent;
	}

	T* bigger(T* item) const {
		T* node = (T*) item->r();
		if (node) {
			while (node->l())
				node = (T*) node->l();
			return node;
		}
		node = item;
		T* parent = (T*) node->p();
		while (parent && parent->r() == node) {
			node = parent;
			parent = (T*) node->p();
		}
		return parent;
	}

	T* get(Key key) const {
		T* node = (T*) _root;
		while (node) {
			int r = compare(key, node->getKey());
			if (r == 0)
				break;
			if (r < 0)
				node = (T*) node->l();
			else
				node = (T*) node->r();
		}
		return node;
	}
};

template<typename T, const char* fmt>
struct ValueSet: Map<T, ValueMapItem<T, fmt, T> > {
};

template<class T>
struct ObjectSet: Map<T, ObjectMapItem<T, T> > {
};

struct StringSet: Map<String, StringMapItem<String> > {
};

}
