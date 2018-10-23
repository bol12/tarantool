#!/usr/bin/env tarantool

swim = require('swim')
fiber = require('fiber')
this_uri = require('os').getenv("LISTEN")

uuids = {'00000000-0000-0000-0000-000000000001', '00000000-0000-0000-0000-000000000002'}

box.cfg{}

require('console').listen(os.getenv('ADMIN'))
