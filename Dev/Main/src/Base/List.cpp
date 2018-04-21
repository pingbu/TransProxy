#include "Debug.h"
#include "List.h"

namespace Utils {

void _List::__insertHead(ListItem* item) {
	ASSERT(item != NULL);
	item->prev(NULL);
	item->next(_first);
	if (_first)
		_first->prev(item);
	else
		_last = item;
	_first = item;
	++_count;
}

void _List::__insertTail(ListItem* item) {
	ASSERT(item != NULL);
	item->prev(_last);
	item->next(NULL);
	if (_last)
		_last->next(item);
	else
		_first = item;
	_last = item;
	++_count;
}

void _List::__insertBefore(ListItem* item, ListItem* ref) {
	ASSERT(item != NULL);
	if (ref == NULL) {
		__insertHead(item);
		return;
	}
	ListItem* prev = ref->prev();
	item->prev(prev);
	item->next(ref);
	ref->prev(item);
	if (prev)
		prev->next(item);
	else
		_first = item;
	++_count;
}

void _List::__insertAfter(ListItem* item, ListItem* ref) {
	ASSERT(item != NULL);
	if (ref == NULL) {
		__insertTail(item);
		return;
	}
	ListItem* next = ref->next();
	item->prev(ref);
	item->next(next);
	if (next)
		next->prev(item);
	else
		_last = item;
	ref->next(item);
	++_count;
}

void _List::__remove(ListItem* item) {
	ASSERT(item != NULL);
	ListItem* prev = item->prev();
	ListItem* next = item->next();
	if (prev)
		prev->next(next);
	else
		_first = next;
	if (next)
		next->prev(prev);
	else
		_last = prev;
	--_count;
}

}
