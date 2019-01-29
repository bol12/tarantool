#ifndef TARANTOOL_SWIM_H_INCLUDED
#define TARANTOOL_SWIM_H_INCLUDED
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
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct info_handler;
struct swim;
struct tt_uuid;
struct swim_iterator;
struct swim_member;

/**
 * Create a new SWIM instance. Do not bind to a port or set any
 * parameters. Allocation and initialization only.
 */
struct swim *
swim_new(void);

/** Check if a swim instance is configured. */
bool
swim_is_configured(const struct swim *swim);

/**
 * Configure or reconfigure a SWIM instance.
 *
 * @param swim SWIM instance to configure.
 * @param uri URI in the format "ip:port".
 * @param heartbeat_rate Rate of sending round messages. It does
 *        not mean that each member will be checked each
 *        @heartbeat_rate seconds. It is rather the protocol
 *        speed. Protocol period depends on member count and
 *        @heartbeat_rate.
 * @param ack_timeout Time in seconds after which a ping is
 *        considered to be unacknowledged.
 * @param uuid UUID of this instance. Must be unique over the
 *        cluster.
 *
 * @retval 0 Success.
 * @retval -1 Error. Memory, not unique UUID, system error.
 */
int
swim_cfg(struct swim *swim, const char *uri, double heartbeat_rate,
	 double ack_timeout, const struct tt_uuid *uuid);

/** Set payload to disseminate over the cluster. */
int
swim_set_payload(struct swim *swim, const char *payload, int payload_size);

/**
 * Stop listening and broadcasting messages, cleanup all internal
 * structures, free memory.
 */
void
swim_delete(struct swim *swim);

/** Add a new member. */
int
swim_add_member(struct swim *swim, const char *uri, const struct tt_uuid *uuid);

/** Silently remove a member from member table. */
int
swim_remove_member(struct swim *swim, const struct tt_uuid *uuid);

/**
 * Send a ping to this address. If an ACK is received, the member
 * will be added.
 */
int
swim_probe_member(struct swim *swim, const char *uri);

/**
 * Broadcast a ping to all interfaces on a specified port. If a
 * port is < 0, then a port of the SWIM instance is used.
 */
int
swim_broadcast(struct swim *swim, int port);

/** Dump member statuses into @a info. */
void
swim_info(struct swim *swim, struct info_handler *info);

/**
 * Gracefully leave the cluster, broadcast a notification.
 * Members, received it, will remove a record about this instance
 * from their tables, and will not consider it to be dead.
 */
void
swim_quit(struct swim *swim);

/** Get a SWIM member, describing this instance. */
const struct swim_member *
swim_self(struct swim *swim);

/**
 * Find a member by its UUID in the local member table.
 * @retval NULL Not found.
 * @retval not NULL A member.
 */
const struct swim_member *
swim_member_by_uuid(struct swim *swim, const struct tt_uuid *uuid);

/**
 * Open an iterator to scan the whole member table. The iterator
 * is not stable. It means, that a caller can not yield between
 * open and close - iterator position can be lost.
 */
struct swim_iterator *
swim_iterator_open(struct swim *swim);

/**
 * Get a next SWIM member.
 * @retval NULL EOF.
 * @retval not NULL A valid member.
 */
const struct swim_member *
swim_iterator_next(struct swim_iterator *iterator);

/** Close an iterator. */
void
swim_iterator_close(struct swim_iterator *iterator);

/** Member's URI. */
const char *
swim_member_uri(const struct swim_member *member);

/** Member's UUID. */
const struct tt_uuid *
swim_member_uuid(const struct swim_member *member);

/** Member's payload. */
const char *
swim_member_payload(const struct swim_member *member, int *size);

#if defined(__cplusplus)
}
#endif

#endif /* TARANTOOL_SWIM_H_INCLUDED */
