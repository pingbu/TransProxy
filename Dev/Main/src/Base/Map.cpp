//#define LOG_TAG "Map"

#include <stdlib.h>
#include "Debug.h"
#include "Map.h"

namespace Utils {

void _Map::__free(_MapItem* root) {
	if (root) {
		__free(root->l());
		__free(root->r());
		delete root;
	}
}

void _Map::__dump(const _MapItem* item, const char* p, const char* pl,
		const char* pr) const {
	THROW_IF(::strlen(p) > 80, new Exception("Overflow!!!!!!"));
	if (item != NULL) {
		__dump(item->l(), String::format("%s  +--", pl),
				String::format("%s    ", pl), String::format("%s  | ", pl));
		Log::v("%s(%d) %s", p, item->height(), __getKeyString(item).sz());
		__dump(item->r(), String::format("%s  +--", pr),
				String::format("%s  | ", pr), String::format("%s    ", pr));
	}
}

void _Map::__checkOrder() const {
	if (!_name)
		return;
	__checkParent(NULL, _root);
	const _MapItem *minNode, *maxNode;
	if (_root && !__checkMinMax(_root, minNode, maxNode)) {
		Log::e("Map[%s] __checkOrder FAILED!!!", _name.sz());
		dump();
		::abort();
	}
}

void _Map::__checkParent(const _MapItem* parent, const _MapItem* node) const {
	if (node) {
		if (node->p() != parent) {
			Log::e("Map[%s] __checkParent FAILED at node '%s'!!!", _name.sz(),
					__getKeyString(node).sz());
			dump();
			::abort();
		}
		__checkParent(node, node->l());
		__checkParent(node, node->r());
	}
}

bool _Map::__checkMinMax(const _MapItem* root, const _MapItem*& minNode,
		const _MapItem*& maxNode) const {
	const _MapItem *subMin, *subMax;
	if (root->l()) {
		if (!__checkMinMax(root->l(), subMin, subMax)
				|| __compare(root, subMax) < 0)
			return false;
		minNode = subMin;
	} else {
		minNode = root;
	}
	if (root->r()) {
		if (!__checkMinMax(root->r(), subMin, subMax)
				|| __compare(root, subMin) > 0)
			return false;
		maxNode = subMax;
	} else {
		maxNode = root;
	}
	return true;
}

bool _Map::__insert(_MapItem* parent, _MapItem*& root, _MapItem* const node) {
	int c0 = __count(root);
	bool ret;
	if (root == NULL) {
		// found empty place, insert node
		node->p(parent);
		node->l(node->r(NULL));
		node->height(1);
		root = node;
		++_size;
		ret = true;
	} else if (root == node) {
		ret = false; // node in tree ready, ignore
	} else {
		int c = __compare(node, root);
		if (c < 0) {
			ret = __insert(root, root->l(), node); // insert into left subtree
			if (ret)
				__adjust(root);
		} else {
			ret = __insert(root, root->r(), node); // insert into right subtree
			if (ret)
				__adjust(root);
		}
	}
	int c1 = __count(root);
	if (c1 - c0 != 1) {
		Log::e("Map[%s] insert node ERROR!!! c0=%d, c1=%d", _name.sz(), c0, c1);
		__dump(root, "--", " ", " ");
		::abort();
	}
	return ret;
}

bool _Map::__remove(_MapItem*& root, _MapItem* const node) {
	bool ret;
	if (root == NULL) {
		ret = false;
	} else if (root == node) {
		// found, remove it
		if (_name)
			Log::d("found, remove it");
		__remove(root);
		--_size;
		ret = true;
	} else {
		ret = false;
		int c = __compare(node, root);
		if (c <= 0) {
			if (_name)
				Log::d("remove from left subtree - %s",
						__getKeyString(root->l()).sz());
			ret = __remove(root->l(), node); // remove from left subtree
		}
		if (!ret && c >= 0) {
			if (_name)
				Log::d("remove from right subtree - %s",
						__getKeyString(root->r()).sz());
			ret = __remove(root->r(), node); // remove from right subtree
		}
		if (ret) {
			if (_name)
				Log::d("node removed, adjust");
			__adjust(root);
		}
	}
	return ret;
}

void _Map::__remove(_MapItem*& node) {
	_MapItem* l = node->l();
	_MapItem* r = node->r();
	if (l == NULL) {
		if (_name)
			Log::d("no left child, lift right - %s", __getKeyString(r).sz());
		if (r)
			r->p(node->p());
		node = r;
	} else if (r == NULL) {
		if (_name)
			Log::d("no right child, lift left - %s", __getKeyString(l).sz());
		if (l)
			l->p(node->p());
		node = l;
	} else {
		_MapItem* parent = node->p();
		int h = node->height();
		int hl = l->height();
		int hr = r->height();
		if (hl > hr) {
			if (_name)
				Log::d("left child is higher, remove max from it - %s",
						__getKeyString(l).sz());
			node = __removeMax(l);
			hl = l == NULL ? 0 : l->height();
		} else {
			if (_name)
				Log::d("right child is higher, remove min from it - %s",
						__getKeyString(r).sz());
			node = __removeMin(r);
			hr = r == NULL ? 0 : r->height();
		}
		if (l)
			l->p(node);
		if (r)
			r->p(node);
		node->p(parent);
		node->l(l);
		node->r(r);
		node->height(max(hl, hr) + 1);
		if (node->height() < h - 1 || node->height() > h) {
			Log::e("Map[%s] remove ERROR!!! Height decrease more than 1",
					_name.sz());
			__dump(node, "--", " ", " ");
			::abort();
		}
		if (::abs(hl - hr) > 1) {
			Log::e("Map[%s] remove ERROR!!! Absolute balance higher than 1",
					_name.sz());
			__dump(node, "--", " ", " ");
			::abort();
		}
	}
}

_MapItem* _Map::__removeMin(_MapItem*& root) {
	_MapItem* ret;
	if (root->l() == NULL) {
		if (_name)
			Log::d("no left child, min found, remove it - %s",
					__getKeyString(root).sz());
		ret = root;
		root = root->r();
	} else {
		if (_name)
			Log::d("left child exists, remove min from it - %s",
					__getKeyString(root->l()).sz());
		ret = __removeMin(root->l());
		if (root->l())
			root->l()->p(root);
	}
	if (_name)
		Log::d("try to adjust after removed min - %s",
				__getKeyString(ret).sz());
	__adjust(root);
	return ret;
}

_MapItem* _Map::__removeMax(_MapItem*& root) {
	_MapItem* ret;
	if (root->r() == NULL) {
		if (_name)
			Log::d("no right child, max found, remove it - %s",
					__getKeyString(root).sz());
		ret = root;
		root = root->l();
	} else {
		if (_name)
			Log::d("right child exists, remove max from it - %s",
					__getKeyString(root->r()).sz());
		ret = __removeMax(root->r());
		if (root->r())
			root->r()->p(root);
	}
	if (_name)
		Log::d("try to adjust after removed max - %s",
				__getKeyString(ret).sz());
	__adjust(root);
	return ret;
}

void _Map::__adjust(_MapItem*& root) {
	if (root) {
		_MapItem* l = root->l();
		_MapItem* r = root->r();
		int hl = l ? l->height() : 0;
		int hr = r ? r->height() : 0;

		root->height(max(hl, hr) + 1);

		int balance = hr - hl;
		if (::abs(balance) > 2) {
			Log::e(
					"Map[%s] adjest ERROR!!! Absolute balance higher than 2 before adjust",
					_name.sz());
			__dump(root, "--", " ", " ");
			::abort();
		}

		if (balance == -2) {
			int hll = l->l() == NULL ? 0 : l->l()->height();
			int hlr = l->r() == NULL ? 0 : l->r()->height();
			if (hll >= hlr) {
				// LL
				if (_name)
					Log::d("LL");
				if (l->r())
					l->r()->p(root);
				root->l(l->r());
				root->height(max(hlr, hr) + 1);
				l->r(root);
				l->height(max(hll, root->height()) + 1);
				l->p(root->p());
				root->p(l);
				root = l;
			} else {
				// LR
				if (_name)
					Log::d("LR");
				_MapItem* lr = l->r();
				int hlrl = lr->l() == NULL ? 0 : lr->l()->height();
				int hlrr = lr->r() == NULL ? 0 : lr->r()->height();
				if (lr->l())
					lr->l()->p(l);
				l->r(lr->l());
				l->height(max(hll, hlrl) + 1);
				l->p(lr);
				if (lr->r())
					lr->r()->p(root);
				root->l(lr->r());
				root->height(max(hlrr, hr) + 1);
				lr->l(l);
				lr->r(root);
				lr->height(max(l->height(), root->height()) + 1);
				lr->p(root->p());
				root->p(lr);
				root = lr;
			}
		} else if (balance == 2) {
			int hrl = r->l() == NULL ? 0 : r->l()->height();
			int hrr = r->r() == NULL ? 0 : r->r()->height();
			if (hrl > hrr) {
				// RL
				if (_name)
					Log::d("RL");
				_MapItem* rl = r->l();
				int hrll = rl->l() == NULL ? 0 : rl->l()->height();
				int hrlr = rl->r() == NULL ? 0 : rl->r()->height();
				if (rl->r())
					rl->r()->p(r);
				r->l(rl->r());
				r->height(max(hrlr, hrr) + 1);
				r->p(rl);
				if (rl->l())
					rl->l()->p(root);
				root->r(rl->l());
				root->height(max(hl, hrll) + 1);
				rl->r(r);
				rl->l(root);
				rl->height(max(root->height(), r->height()) + 1);
				rl->p(root->p());
				root->p(rl);
				root = rl;
			} else {
				// RR
				if (_name)
					Log::d("RR");
				if (r->l())
					r->l()->p(root);
				root->r(r->l());
				root->height(max(hl, hrl) + 1);
				r->l(root);
				r->height(max(hrr, root->height()) + 1);
				r->p(root->p());
				root->p(r);
				root = r;
			}
		}
	}
}

}
