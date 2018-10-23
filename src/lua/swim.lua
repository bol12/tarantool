local swim = require('swim')
local internal_methods = swim.methods
swim.methods = nil
local ffi = require('ffi')
ffi.cdef("struct swim;")
local swim_t = ffi.typeof("struct swim")

ffi.metatype(swim_t, {
    __index = function(s, key)
        return internal_methods[key]
    end
})
