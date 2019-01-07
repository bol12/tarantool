#!/usr/bin/env tarantool

local tap = require('tap')
local ffi = require('ffi')
local key_def = require('key_def')

local usage_error = 'Bad params, use: key_def.new({' ..
                    '{fieldno = fieldno, type = type' ..
                    '[, is_nullable = <boolean>]' ..
                    '[, collation_id = <number>]' ..
                    '[, collation = <string>]}, ...}'

local function coll_not_found(fieldno, collation)
    if type(collation) == 'number' then
        return ('Wrong index options (field %d): ' ..
               'collation was not found by ID'):format(fieldno)
    end

    return ('Unknown collation: "%s"'):format(collation)
end

local cases = {
    -- Cases to call before box.cfg{}.
    {
        'Pass a field on an unknown type',
        parts = {{
            fieldno = 2,
            type = 'unknown',
        }},
        exp_err = 'Unknown field type: unknown',
    },
    {
        'Try to use collation_id before box.cfg{}',
        parts = {{
            fieldno = 1,
            type = 'string',
            collation_id = 2,
        }},
        exp_err = coll_not_found(1, 2),
    },
    {
        'Try to use collation before box.cfg{}',
        parts = {{
            fieldno = 1,
            type = 'string',
            collation = 'unicode_ci',
        }},
        exp_err = coll_not_found(1, 'unicode_ci'),
    },
    function()
        -- For collations.
        box.cfg{}
    end,
    -- Cases to call after box.cfg{}.
    {
        'Try to use both collation_id and collation',
        parts = {{
            fieldno = 1,
            type = 'string',
            collation_id = 2,
            collation = 'unicode_ci',
        }},
        exp_err = 'Conflicting options: collation_id and collation',
    },
    {
        'Unknown collation_id',
        parts = {{
            fieldno = 1,
            type = 'string',
            collation_id = 42,
        }},
        exp_err = coll_not_found(1, 42),
    },
    {
        'Unknown collation name',
        parts = {{
            fieldno = 1,
            type = 'string',
            collation = 'unknown',
        }},
        exp_err = 'Unknown collation: "unknown"',
    },
    {
        'Bad parts parameter type',
        parts = 1,
        exp_err = usage_error,
    },
    {
        'No parameters',
        params = {},
        exp_err = usage_error,
    },
    {
        'Two parameters',
        params = {{}, {}},
        exp_err = usage_error,
    },
    {
        'Success case; zero parts',
        parts = {},
        exp_err = nil,
    },
    {
        'Success case; one part',
        parts = {
            fieldno = 1,
            type = 'string',
        },
        exp_err = nil,
    },
}

local test = tap.test('key_def')

test:plan(#cases - 1)
for _, case in ipairs(cases) do
    if type(case) == 'function' then
        case()
    else
        local ok, res
        if case.params then
            ok, res = pcall(key_def.new, unpack(case.params))
        else
            ok, res = pcall(key_def.new, case.parts)
        end
        if case.exp_err == nil then
            ok = ok and type(res) == 'cdata' and
                ffi.istype('struct key_def', res)
            test:ok(ok, case[1])
        else
            local err = tostring(res) -- cdata -> string
            test:is_deeply({ok, err}, {false, case.exp_err}, case[1])
        end
    end
end

os.exit(test:check() and 0 or 1)
