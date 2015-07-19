#ifndef _MIN_HEAP_H_
#define _MIN_HEAP_H_

#include <time.h>

typedef struct {
    int id;
	int min_heap_idx;
	time_t timeout;
	void *data;

} timer_event_t;

typedef struct min_heap {
	timer_event_t **pool;
	unsigned num, size;
} min_heap_t;

static inline void min_heap_ctor(min_heap_t * s);
static inline void min_heap_dtor(min_heap_t * s);
static inline void min_heap_elem_init(timer_event_t * e, int timeout, void *dat);
static inline int min_heap_elt_is_top(const timer_event_t * e);
static inline int min_heap_elem_greater(timer_event_t * a, timer_event_t * b);
static inline int min_heap_empty(min_heap_t * s);
static inline unsigned min_heap_size(min_heap_t * s);
static inline timer_event_t *min_heap_top(min_heap_t * s);
static inline int min_heap_reserve(min_heap_t * s, unsigned n);
static inline int min_heap_push(min_heap_t * s, timer_event_t * e);
static inline timer_event_t *min_heap_pop(min_heap_t * s);
static inline int min_heap_erase(min_heap_t * s, timer_event_t * e);
static inline void min_heap_shift_up_(min_heap_t * s, unsigned hole_index, timer_event_t * e);
static inline void min_heap_shift_down_(min_heap_t * s, unsigned hole_index, timer_event_t * e);

int min_heap_elem_greater(timer_event_t * a, timer_event_t * b)
{
	return (a->timeout > b->timeout);
}

void min_heap_ctor(min_heap_t * s)
{
	s->pool = 0;
	s->num = 0;
	s->size = 0;
}

void min_heap_dtor(min_heap_t * s)
{
	if (s->pool)
		M_free(s->pool);
}

void min_heap_elem_init(timer_event_t * e, int timeout, void *dat)
{
	e->min_heap_idx = -1;
    e->timeout = timeout;
    e->data = dat;
}

int min_heap_empty(min_heap_t * s)
{
	return 0u == s->num;
}

unsigned min_heap_size(min_heap_t * s)
{
	return s->num;
}

timer_event_t *min_heap_top(min_heap_t * s)
{
	return s->num ? *s->pool : 0;
}

int min_heap_push(min_heap_t * s, timer_event_t * e)
{
    e->min_heap_idx = -1;
	if (min_heap_reserve(s, s->num + 1))
		return -1;
	min_heap_shift_up_(s, s->num++, e);
	return 0;
}

timer_event_t *min_heap_pop(min_heap_t * s)
{
	if (s->num) {
		timer_event_t *e = *s->pool;
		min_heap_shift_down_(s, 0u, s->pool[--s->num]);
		e->min_heap_idx = -1;
		return e;
	}
	return 0;
}

int min_heap_elt_is_top(const timer_event_t * e)
{
	return e->min_heap_idx == 0;
}

int min_heap_erase(min_heap_t * s, timer_event_t * e)
{
	if (-1 != e->min_heap_idx) {
		timer_event_t *last = s->pool[--s->num];
		unsigned parent = (e->min_heap_idx - 1) / 2;
		/* we replace e with the last element in the heap.  We might need to
		   shift it upward if it is less than its parent, or downward if it is
		   greater than one or both its children. Since the children are known
		   to be less than the parent, it can't need to shift both up and
		   down. */
		if (e->min_heap_idx > 0 && min_heap_elem_greater(s->pool[parent], last))
			min_heap_shift_up_(s, e->min_heap_idx, last);
		else
			min_heap_shift_down_(s, e->min_heap_idx, last);
		e->min_heap_idx = -1;
		return 0;
	}
	return -1;
}

int min_heap_reserve(min_heap_t * s, unsigned n)
{
	if (s->size < n) {
		timer_event_t **p;
		unsigned a = s->size ? s->size * 2 : 8;
		if (a < n)
			a = n;
		if (!(p = (timer_event_t **) M_realloc(s->pool, a * sizeof *p)))
			return -1;
		s->pool = p;
		s->size = a;
	}
	return 0;
}

void min_heap_shift_up_(min_heap_t * s, unsigned hole_index, timer_event_t * e)
{
	unsigned parent = (hole_index - 1) / 2;
	while (hole_index && min_heap_elem_greater(s->pool[parent], e)) {
		(s->pool[hole_index] = s->pool[parent])->min_heap_idx = hole_index;
		hole_index = parent;
		parent = (hole_index - 1) / 2;
	}
	(s->pool[hole_index] = e)->min_heap_idx = hole_index;
}

void min_heap_shift_down_(min_heap_t * s, unsigned hole_index, timer_event_t * e)
{
	unsigned min_child = 2 * (hole_index + 1);
	while (min_child <= s->num) {
		min_child -= min_child == s->num || min_heap_elem_greater(s->pool[min_child], s->pool[min_child - 1]);
		if (!(min_heap_elem_greater(e, s->pool[min_child])))
			break;
		(s->pool[hole_index] = s->pool[min_child])->min_heap_idx = hole_index;
		hole_index = min_child;
		min_child = 2 * (hole_index + 1);
	}
	(s->pool[hole_index] = e)->min_heap_idx = hole_index;
}

#endif /* _MIN_HEAP_H_ */
