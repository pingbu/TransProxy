#include <stddef.h>
#include "Debug.h"
#include "Math.h"

#pragma once

namespace Utils {

class ListItem {
	ListItem *_prev, *_next;
public:
	virtual ~ListItem() {
	}
	ListItem*& prev() THROWS {
		ASSERT(this);
		return _prev;
	}
	ListItem*& next() THROWS {
		ASSERT(this);
		return _next;
	}
	ListItem* prev() const THROWS {
		ASSERT(this);
		return _prev;
	}
	ListItem* next() const THROWS {
		ASSERT(this);
		return _next;
	}
	ListItem* prev(ListItem* prev) THROWS {
		ASSERT(this);
		return (_prev = prev);
	}
	ListItem* next(ListItem* next) THROWS {
		ASSERT(this);
		return (_next = next);
	}
};

template<class T>
class ListItemPtr: public ListItem {
	T* _p;
public:
	ListItemPtr(T* p) {
		_p = p;
	}
	operator T*() const {
		return this == NULL ? NULL : _p;
	}
	T* operator->() const {
		return this == NULL ? NULL : _p;
	}
};

template<class T>
class ValueListItem: public ListItem {
	T _value;
public:
	ValueListItem(const T& value) {
		_value = value;
	}
	operator const T&() const {
		ASSERT(this);
		return _value;
	}
};

class StringListItem: public ListItem {
	String _value;
public:
	StringListItem(const char* value) :
			_value(value) {
	}
	operator const char*() const {
		return this == NULL ? NULL : _value.sz();
	}
};

class _List {
protected:
	String _name;
	ListItem *_first = NULL, *_last = NULL;
	size_t _count;
	_List(const char* name) :
			_name(name), _count(0) {
	}
	virtual ~_List() {
		clear();
	}
	void __insertHead(ListItem* item);
	void __insertTail(ListItem* item);
	void __insertBefore(ListItem* item, ListItem* ref);
	void __insertAfter(ListItem* item, ListItem* ref);
	void __remove(ListItem* item);
public:
	size_t size() const {
		return _count;
	}
	bool isEmpty() const {
		return _first == NULL;
	}
	void clear() {
		for (ListItem* item = _first; item;) {
			ListItem* next = item->next();
			delete item;
			item = next;
		}
		_first = _last = NULL;
		_count = 0;
	}
};

template<class T>
class List: public _List {
public:
	List(const char* name = NULL) :
			_List(name) {
	}
	T* first() const {
		return (T*) _first;
	}
	T* last() const {
		return (T*) _last;
	}
	T* prev(T* item) const {
		return (T*) item->prev();
	}
	T* next(T* item) const {
		return (T*) item->next();
	}
	void insertHead(T* item) {
		__insertHead(item);
	}
	void insertTail(T* item) {
		__insertTail(item);
	}
	void insertBefore(T* item, T* ref) {
		__insertBefore(item, ref);
	}
	void insertAfter(T* item, T* ref) {
		__insertAfter(item, ref);
	}
	void remove(T* item) {
		__remove(item);
	}
};

}
