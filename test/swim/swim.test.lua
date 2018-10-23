test_run = require('test_run').new()
test_run:cmd("push filter '127.0.0.1:.*' to '127.0.0.1:<port>'")
--
-- gh-3234: SWIM gossip protocol.
--

s = swim.new()
s:delete()
s = nil
-- Check that SWIM gc can cope with manually deleted instance.
_ = collectgarbage()

s = swim.new()
s = nil
-- Check auto gc of active instances.
_ = collectgarbage()

uri = '127.0.0.1:0'

s1, err = swim.new({server = uri, uuid = uuids[1], heartbeat = 0.01})
err
s2, err = swim.new({server = uri, uuid = uuids[2], heartbeat = 0.01})
err
s1:info()
s2:info()

s1:delete()
s2:delete()
test_run:cmd("clear filter")
