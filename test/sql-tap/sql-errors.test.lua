#!/usr/bin/env tarantool
test = require("sqltester")
test:plan(20)

test:execsql([[
	CREATE TABLE t0 (i INT PRIMARY KEY);
	CREATE VIEW v0 AS SELECT * FROM t0;
]])

test:do_catchsql_test(
	"sql-errors-1.1",
	[[
		ANALYZE v0;
	]], {
		-- <sql-errors-1.1>
		1,"ANALYZE statement argument V0 is not a base table"
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
		1,"Failed to create space 'T2': space column count 2001 exceeds the limit (2000)"
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
		CREATE VIEW v7(a,b) AS SELECT * FROM t0;
	]], {
		-- <sql-errors-1.7>
		1,"Failed to create space 'V7': number of aliases doesn't match provided columns"
		-- </sql-errors-1.7>
	})

test:do_catchsql_test(
	"sql-errors-1.8",
	[[
		DROP VIEW t0;
	]], {
		-- <sql-errors-1.8>
		1,"Can't drop space 'T0': use DROP TABLE"
		-- </sql-errors-1.8>
	})

test:do_catchsql_test(
	"sql-errors-1.9",
	[[
		DROP TABLE v0;
	]], {
		-- <sql-errors-1.9>
		1,"Can't drop space 'V0': use DROP VIEW"
		-- </sql-errors-1.9>
	})

test:do_catchsql_test(
	"sql-errors-1.10",
	[[
		CREATE TABLE t10(i INT PRIMARY KEY REFERENCES v0);
	]], {
		-- <sql-errors-1.10>
		1,"Failed to create foreign key constraint 'FK_CONSTRAINT_1_T10': referenced space can't be VIEW"
		-- </sql-errors-1.10>
	})

test:do_catchsql_test(
	"sql-errors-1.11",
	[[
		CREATE VIEW v11 AS SELECT * FROM t0 WHERE i = ?;
	]], {
		-- <sql-errors-1.11>
		1,"Failed to create space 'V11': parameters are not allowed in views"
		-- </sql-errors-1.11>
	})

test:do_catchsql_test(
	"sql-errors-1.12",
	[[
		CREATE INDEX i12 ON v0(i);
	]], {
		-- <sql-errors-1.12>
		1,"Can't create or modify index 'I12' in space 'V0': views can not be indexed"
		-- </sql-errors-1.12>
	})

test:do_catchsql_test(
	"sql-errors-1.13",
	[[
		SELECT 9223372036854775808;
	]], {
		-- <sql-errors-1.13>
		1,"Integer literal 9223372036854775808 exceeds the supported range -9223372036854775808 - 9223372036854775807"
		-- </sql-errors-1.13>
	})

test:do_catchsql_test(
	"sql-errors-1.14",
	[[
		SELECT 0x10000000000000000;
	]], {
		-- <sql-errors-1.14>
		1,"Hex literal 0x10000000000000000 length 17 exceeds the supported limit (16)"
		-- </sql-errors-1.14>
	})

select_statement = 'SELECT i FROM t0 WHERE i = 0'
for i = 1, 200 do
	select_statement = select_statement .. ' OR i = ' .. i
end
select_statement = select_statement .. ';'

test:do_catchsql_test(
	"sql-errors-1.15",
	select_statement,
	{
		-- <sql-errors-1.15>
		1,"Number of nodes in expression tree 201 exceeds the limit (200)"
		-- </sql-errors-1.15>
	})

select_statement = 'SELECT CHAR(1'
for i = 1, 127 do
	select_statement = select_statement .. ', ' .. i
end
select_statement = select_statement .. ');'

test:do_catchsql_test(
	"sql-errors-1.16",
	select_statement,
	{
		-- <sql-errors-1.16>
		1,"Number of arguments to function CHAR 128 exceeds the limit (127)"
		-- </sql-errors-1.16>
	})

select_statement = 'SELECT MAX(1'
for i = 1, 127 do
	select_statement = select_statement .. ', ' .. i
end
select_statement = select_statement .. ');'

test:do_catchsql_test(
	"sql-errors-1.17",
	select_statement,
	{
		-- <sql-errors-1.17>
		1,"Number of arguments to function MAX 128 exceeds the limit (127)"
		-- </sql-errors-1.17>
	})

select_statement = 'SELECT 0'
for i = 1, 30 do
	select_statement = select_statement .. ' UNION ALL SELECT ' .. i
end
select_statement = select_statement .. ';'

test:do_catchsql_test(
	"sql-errors-1.18",
	select_statement,
	{
		-- <sql-errors-1.18>
		1,"The number of UNION or EXCEPT or INTERSECT operations 31 exceeds the limit (30)"
		-- </sql-errors-1.18>
	})

select_statement = 'SELECT 0'
for i = 1, 2000 do
	select_statement = select_statement .. ', ' .. i
end
select_statement = select_statement .. ';'

test:do_catchsql_test(
	"sql-errors-1.19",
	select_statement,
	{
		-- <sql-errors-1.19>
		1,"The number of columns in result set 2001 exceeds the limit (2000)"
		-- </sql-errors-1.19>
	})

select_statement = 'SELECT * FROM t0'
for i = 1, 64 do
	select_statement = select_statement .. ', t0 as t' .. i
end
select_statement = select_statement .. ';'

test:do_catchsql_test(
	"sql-errors-1.20",
	select_statement,
	{
		-- <sql-errors-1.20>
		1,"The number of tables in a join 65 exceeds the limit (64)"
		-- </sql-errors-1.20>
	})

test:finish_test()
