/*
 * Copyright 2010-2019, Tarantool AUTHORS, please see AUTHORS file.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "swim_test_ev.h"
#include "swim_test_transport.h"
#include "trivia/util.h"
#include "swim/swim_ev.h"
#include "tarantool_ev.h"
#define HEAP_FORWARD_DECLARATION
#include "salad/heap.h"
#include "assoc.h"
#include "say.h"
#include <stdbool.h>

static double watch = 0;

static int event_id = 0;

struct swim_test_event {
	int revents;
	struct ev_watcher *watcher;
	double deadline;
	struct heap_node in_events_heap;
	int id;
};

static inline bool
swim_test_event_less(const struct swim_test_event *e1,
		     const struct swim_test_event *e2)
{
	if (e1->deadline == e2->deadline)
		return e1->id < e2->id;
	return e1->deadline < e2->deadline;
}

#define HEAP_NAME events_heap
#define HEAP_LESS(h, e1, e2) swim_test_event_less(e1, e2)
#define heap_value_t struct swim_test_event
#define heap_value_attr in_events_heap
#include "salad/heap.h"

static heap_t events_heap;

static struct mh_i64ptr_t *events_hash;

static void
swim_test_event_new(struct ev_watcher *watcher, double delay, int revents)
{
	/*
	 * Create event. Push into the queue, and the watcher's
	 * list.
	 */
	struct swim_test_event *e =
		(struct swim_test_event *) malloc(sizeof(*e));
	assert(e != NULL);
	e->watcher = watcher;
	e->deadline = swim_time() + delay;
	e->revents = revents;
	e->id = event_id++;
	events_heap_insert(&events_heap, e);
	struct mh_i64ptr_node_t old = {0, NULL}, *old_p = &old;
	struct mh_i64ptr_node_t node = {(uint64_t) watcher, e};
	mh_int_t rc = mh_i64ptr_put(events_hash, &node, &old_p, NULL);
	(void) rc;
	assert(rc != mh_end(events_hash));
	assert(old.val == NULL && old.key == 0);
}

static inline void
swim_test_event_delete(struct swim_test_event *e)
{
	events_heap_delete(&events_heap, e);
	mh_int_t rc = mh_i64ptr_find(events_hash, (uint64_t) e->watcher, NULL);
	assert(rc != mh_end(events_hash));
	mh_i64ptr_del(events_hash, rc, NULL);
	free(e);
}

static struct swim_test_event *
swim_test_event_by_ev(struct ev_watcher *watcher)
{
	mh_int_t rc = mh_i64ptr_find(events_hash, (uint64_t) watcher, NULL);
	if (rc == mh_end(events_hash))
		return NULL;
	return (struct swim_test_event *) mh_i64ptr_node(events_hash, rc)->val;
}

double
swim_time(void)
{
	return watch;
}

void
swim_ev_timer_start(struct ev_loop *loop, struct ev_timer *base)
{
	if (swim_test_event_by_ev((struct ev_watcher *) base) != NULL)
		return;
	/* Create the periodic watcher and one event. */
	swim_test_event_new((struct ev_watcher *) base, base->at, EV_TIMER);
}

void
swim_ev_timer_stop(struct ev_loop *loop, struct ev_timer *base)
{
	/*
	 * Delete the watcher and its events. Should be only one.
	 */
	struct swim_test_event *e =
		swim_test_event_by_ev((struct ev_watcher *) base);
	if (e == NULL)
		return;
	swim_test_event_delete(e);
}

void
swim_do_loop_step(struct ev_loop *loop)
{
	say_verbose("Loop watch %f", watch);
	/*
	 * Take next event. Update global watch. Execute its cb.
	 * Do one loop step for the transport.
	 */
	struct swim_test_event *e = events_heap_top(&events_heap);
	if (e != NULL) {
		assert(e->deadline >= watch);
		watch = e->deadline;
		do {
			int revents = e->revents;
			struct ev_watcher *w = e->watcher;
			swim_test_event_delete(e);
			ev_invoke(loop, w, revents);
			e = events_heap_top(&events_heap);
		} while (e != NULL && e->deadline == watch);
	}
	do {
		swim_transport_do_loop_step(loop);
		if (ev_pending_count(loop) == 0)
			break;
		ev_invoke_pending(loop);
	} while (true);
}

void
swim_test_ev_run_loop(struct ev_loop *loop)
{
	while (true)
		swim_do_loop_step(loop);
}

void
swim_test_ev_reset(void)
{
	struct swim_test_event *e;
	while ((e = events_heap_top(&events_heap)) != NULL)
		swim_test_event_delete(e);
	assert(mh_size(events_hash) == 0);
	event_id = 0;
	watch = 0;
}

void
swim_test_ev_init(void)
{
	events_hash = mh_i64ptr_new();
	assert(events_hash != NULL);
	events_heap_create(&events_heap);
}

void
swim_test_ev_free(void)
{
	swim_test_ev_reset();
	events_heap_destroy(&events_heap);
	mh_i64ptr_delete(events_hash);
}
