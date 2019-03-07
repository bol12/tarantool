#!/usr/bin/env tarantool
test = require("sqltester")
test:plan(7)

test:do_catchsql_test(
	"sql-errors-1.1",
	[[
		CREATE TABLE t1 (i INT PRIMARY KEY);
		CREATE VIEW v1 AS SELECT * FROM t1;
		ANALYZE v1;
	]], {
		-- <sql-errors-1.1>
		1,"ANALYZE statement argument V1 is not a base table"
		-- </sql-errors-1.1>
	})

create_statement = 'CREATE TABLE t2 (i INT PRIMARY KEY'
for i = 1, 2001 do
	create_statement = create_statement .. ', s' .. i .. ' INT'
end
create_statement = create_statement .. ');'

test:do_catchsql_test(
	"sql-errors-1.2",
	create_statement,
	{
		-- <sql-errors-1.2>
		1,"Failed to create space 'T2': too many columns"
		-- </sql-errors-1.2>
	})

test:do_catchsql_test(
	"sql-errors-1.3",
	[[
		CREATE TABLE t3 (i INT PRIMARY KEY, a INT DEFAULT(MAX(i, 1)));
	]], {
		-- <sql-errors-1.3>
		1,"Failed to create space 'T3': default value of column is not constant"
		-- </sql-errors-1.3>
	})

test:do_catchsql_test(
	"sql-errors-1.4",
	[[
		CREATE TABLE t4 (i INT PRIMARY KEY, a INT PRIMARY KEY);
	]], {
		-- <sql-errors-1.4>
		1,"Failed to create space 'T4': too many primary keys"
		-- </sql-errors-1.4>
	})

test:do_catchsql_test(
	"sql-errors-1.5",
	[[
		CREATE TABLE t5 (i TEXT PRIMARY KEY AUTOINCREMENT);
	]], {
		-- <sql-errors-1.5>
		1,"Failed to create space 'T5': AUTOINCREMENT is only allowed on an INTEGER PRIMARY KEY or INT PRIMARY KEY"
		-- </sql-errors-1.5>
	})

test:do_catchsql_test(
	"sql-errors-1.6",
	[[
		CREATE TABLE t6 (i INT);
	]], {
		-- <sql-errors-1.6>
		1,"Failed to create space 'T6': PRIMARY KEY missing"
		-- </sql-errors-1.6>
	})

test:do_catchsql_test(
	"sql-errors-1.7",
	[[
		CREATE TABLE t7 (i INT PRIMARY KEY);
		CREATE VIEW v7(a,b) AS SELECT * FROM t7;
	]], {
		-- <sql-errors-1.7>
		1,"Failed to create space 'V7': number of aliases doesn't match provided columns"
		-- </sql-errors-1.7>
	})

test:finish_test()
