#include <stddef.h>
#include "Debug.h"

#pragma once

namespace Utils {

template<typename T>
struct P {
	T* p;
	P(T* p = NULL) :
			p(p) {
	}
	T* operator->() const {
		ASSERT(p);
		return p;
	}
	operator bool() const {
		return p != NULL;
	}
	bool operator!() const {
		return p == NULL;
	}
	bool isNull() const {
		return p == NULL;
	}
	bool operator==(const T* p2) const {
		return p == p2;
	}
	bool operator!=(const T* p2) const {
		return p != p2;
	}
};

template<typename T>
struct PC {
	const T* p;
	PC(const T* p = NULL) :
			p(p) {
	}
	PC(P<T> p) :
			p(p.p) {
	}
	const T* operator->() const {
		ASSERT(p);
		return p;
	}
	operator bool() const {
		return p != NULL;
	}
	bool operator!() const {
		return p == NULL;
	}
	bool isNull() const {
		return p == NULL;
	}
	bool operator==(const T* p2) const {
		return p == p2;
	}
	bool operator!=(const T* p2) const {
		return p != p2;
	}
};

}
