/*
 * Copyright 2010-2018, Tarantool AUTHORS, please see AUTHORS file.
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

#include "box/lua/key_def.h"

#include <lua.h>
#include <lauxlib.h>
#include "diag.h"
#include "box/key_def.h"
#include "box/box.h"
#include "box/coll_id_cache.h"
#include "lua/utils.h"
#include "box/tuple_format.h" /* TUPLE_INDEX_BASE */

static uint32_t key_def_type_id = 0;

/**
 * Set key_part_def from a table on top of a Lua stack.
 *
 * When successful return 0, otherwise return -1 and set a diag.
 */
static int
luaT_key_def_set_part(struct lua_State *L, struct key_part_def *part)
{
	/* Set part->fieldno. */
	lua_pushstring(L, "fieldno");
	lua_gettable(L, -2);
	if (lua_isnil(L, -1)) {
		diag_set(IllegalParams, "fieldno must not be nil");
		return -1;
	}
	/*
	 * Transform one-based Lua fieldno to zero-based
	 * fieldno to use in key_def_new().
	 */
	part->fieldno = lua_tointeger(L, -1) - TUPLE_INDEX_BASE;
	lua_pop(L, 1);

	/* Set part->type. */
	lua_pushstring(L, "type");
	lua_gettable(L, -2);
	if (lua_isnil(L, -1)) {
		diag_set(IllegalParams, "type must not be nil");
		return -1;
	}
	size_t type_len;
	const char *type_name = lua_tolstring(L, -1, &type_len);
	lua_pop(L, 1);
	part->type = field_type_by_name(type_name, type_len);
	switch (part->type) {
	case FIELD_TYPE_ANY:
	case FIELD_TYPE_ARRAY:
	case FIELD_TYPE_MAP:
		/* Tuple comparators don't support these types. */
		diag_set(IllegalParams, "Unsupported field type: %s",
			 type_name);
		return -1;
	case field_type_MAX:
		diag_set(IllegalParams, "Unknown field type: %s", type_name);
		return -1;
	default:
		/* Pass though. */
		break;
	}

	/* Set part->is_nullable and part->nullable_action. */
	lua_pushstring(L, "is_nullable");
	lua_gettable(L, -2);
	if (lua_isnil(L, -1)) {
		part->is_nullable = false;
		part->nullable_action = ON_CONFLICT_ACTION_DEFAULT;
	} else {
		part->is_nullable = lua_toboolean(L, -1);
		part->nullable_action = ON_CONFLICT_ACTION_NONE;
	}
	lua_pop(L, 1);

	/*
	 * Set part->coll_id using collation_id.
	 *
	 * The value will be checked in key_def_new().
	 */
	lua_pushstring(L, "collation_id");
	lua_gettable(L, -2);
	if (lua_isnil(L, -1))
		part->coll_id = COLL_NONE;
	else
		part->coll_id = lua_tointeger(L, -1);
	lua_pop(L, 1);

	/* Set part->coll_id using collation. */
	lua_pushstring(L, "collation");
	lua_gettable(L, -2);
	if (!lua_isnil(L, -1)) {
		/* Check for conflicting options. */
		if (part->coll_id != COLL_NONE) {
			diag_set(IllegalParams, "Conflicting options: "
				 "collation_id and collation");
			return -1;
		}

		size_t coll_name_len;
		const char *coll_name = lua_tolstring(L, -1, &coll_name_len);
		struct coll_id *coll_id = coll_by_name(coll_name,
						       coll_name_len);
		if (coll_id == NULL) {
			diag_set(IllegalParams, "Unknown collation: \"%s\"",
				 coll_name);
			return -1;
		}
		part->coll_id = coll_id->id;
	}
	lua_pop(L, 1);

	/* Set part->sort_order. */
	part->sort_order = SORT_ORDER_ASC;

	/* Set part->path. */
	part->path = NULL;

	return 0;
}

struct key_def *
check_key_def(struct lua_State *L, int idx)
{
	if (lua_type(L, idx) != LUA_TCDATA)
		return NULL;

	uint32_t cdata_type;
	struct key_def **key_def_ptr = luaL_checkcdata(L, idx, &cdata_type);
	if (key_def_ptr == NULL || cdata_type != key_def_type_id)
		return NULL;
	return *key_def_ptr;
}

/**
 * Free a key_def from a Lua code.
 */
static int
lbox_key_def_gc(struct lua_State *L)
{
	struct key_def *key_def = check_key_def(L, 1);
	if (key_def == NULL)
		return 0;
	box_key_def_delete(key_def);
	return 0;
}

/**
 * Create a new key_def from a Lua table.
 *
 * Expected a table of key parts on the Lua stack. The format is
 * the same as box.space.<...>.index.<...>.parts or corresponding
 * net.box's one.
 *
 * Return the new key_def as cdata.
 */
static int
lbox_key_def_new(struct lua_State *L)
{
	if (lua_gettop(L) != 1 || lua_istable(L, 1) != 1)
		return luaL_error(L, "Bad params, use: key_def.new({"
				  "{fieldno = fieldno, type = type"
				  "[, is_nullable = <boolean>]"
				  "[, collation_id = <number>]"
				  "[, collation = <string>]}, ...}");

	uint32_t part_count = lua_objlen(L, 1);
	const ssize_t parts_size = sizeof(struct key_part_def) * part_count;
	struct key_part_def *parts = malloc(parts_size);
	if (parts == NULL) {
		diag_set(OutOfMemory, parts_size, "malloc", "parts");
		return luaT_error(L);
	}

	for (uint32_t i = 0; i < part_count; ++i) {
		lua_pushinteger(L, i + 1);
		lua_gettable(L, 1);
		if (luaT_key_def_set_part(L, &parts[i]) != 0) {
			free(parts);
			return luaT_error(L);
		}
	}

	struct key_def *key_def = key_def_new(parts, part_count);
	free(parts);
	if (key_def == NULL)
		return luaT_error(L);

	*(struct key_def **) luaL_pushcdata(L, key_def_type_id) = key_def;
	lua_pushcfunction(L, lbox_key_def_gc);
	luaL_setcdatagc(L, -2);

	return 1;
}

LUA_API int
luaopen_key_def(struct lua_State *L)
{
	luaL_cdef(L, "struct key_def;");
	key_def_type_id = luaL_ctypeid(L, "struct key_def&");

	/* Export C functions to Lua. */
	static const struct luaL_Reg meta[] = {
		{"new", lbox_key_def_new},
		{NULL, NULL}
	};
	luaL_register_module(L, "key_def", meta);

	return 1;
}
