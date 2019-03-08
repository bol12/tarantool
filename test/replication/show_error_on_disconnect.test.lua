--
-- gh-3365: display an error in upstream on downstream failure.
-- Create a gap in LSN to cause replica's failure.
-- The goal here is to see same error message on both side.
--
test_run = require('test_run').new()
SERVERS = {'show_error_quorum1', 'show_error_quorum2'}

-- Deploy a cluster.
test_run:create_cluster(SERVERS, "replication", {args="20 50"})
test_run:wait_fullmesh(SERVERS)
test_run:cmd("switch show_error_quorum1")
repl = box.cfg.replication
box.cfg{replication = ""}
test_run:cmd("switch show_error_quorum2")
box.space.test:insert{1}
box.snapshot()
box.space.test:insert{2}
box.snapshot()

-- Manually remove all xlogs on show_error_quorum2 to break replication to show_error_quorum1.
fio = require('fio')
for _, path in ipairs(fio.glob(fio.pathjoin(box.cfg.wal_dir, '*.xlog'))) do fio.unlink(path) end

box.space.test:insert{3}

-- Check error reporting.
test_run:cmd("switch show_error_quorum1")
box.cfg{replication = repl}
require('fiber').sleep(0.1)
box.space.test:select()
other_id = box.info.id % 2 + 1
test_run:wait_cond(function() return box.info.replication[other_id].upstream.status == 'stopped' end, 10)
box.info.replication[other_id].upstream.message:match("Missing")
test_run:cmd("switch show_error_quorum2")
box.space.test:select()
other_id = box.info.id % 2 + 1
test_run:wait_cond(function() return box.info.replication[other_id].upstream.status == 'follow' end, 10)
box.info.replication[other_id].upstream.message
test_run:wait_cond(function() return box.info.replication[other_id].downstream.status == 'stopped' end, 10)
box.info.replication[other_id].downstream.message:match("Missing")
test_run:cmd("switch default")
-- Cleanup.
test_run:drop_cluster(SERVERS)
