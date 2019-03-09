env = require('test_run')
test_run = env.new()

-- gh-3018: typeless columns are prohibited.
--
box.sql.execute("CREATE TABLE t1 (id PRIMARY KEY);")
box.sql.execute("CREATE TABLE t1 (a, id INT PRIMARY KEY);")
box.sql.execute("CREATE TABLE t1 (id PRIMARY KEY, a INT);")
box.sql.execute("CREATE TABLE t1 (id INT PRIMARY KEY, a);")
box.sql.execute("CREATE TABLE t1 (id INT PRIMARY KEY, a INT, b UNIQUE);")

-- gh-3104: real type is stored in space format.
--
box.sql.execute("CREATE TABLE t1 (id TEXT PRIMARY KEY, a REAL, b INT, c TEXT, d SCALAR);")
box.space.T1:format()
box.sql.execute("CREATE VIEW v1 AS SELECT b + a, b - a FROM t1;")
box.space.V1:format()

-- gh-2494: index's part also features correct declared type.
--
box.sql.execute("CREATE INDEX i1 ON t1 (a);")
box.sql.execute("CREATE INDEX i2 ON t1 (b);")
box.sql.execute("CREATE INDEX i3 ON t1 (c);")
box.sql.execute("CREATE INDEX i4 ON t1 (id, c, b, a, d);")
box.space.T1.index.I1.parts
box.space.T1.index.I2.parts
box.space.T1.index.I3.parts
box.space.T1.index.I4.parts

box.sql.execute("DROP VIEW v1;")
box.sql.execute("DROP TABLE t1;")

-- gh-3906: data of type BOOL is displayed as should
-- during SQL SELECT.
--
format = {{ name = 'ID', type = 'unsigned' }, { name = 'A', type = 'boolean' }}
sp = box.schema.space.create("TEST", { format = format } )
i = sp:create_index('primary', {parts = {1, 'unsigned' }})
sp:insert({1, true})
sp:insert({2, false})
box.sql.execute("SELECT * FROM test")
sp:drop()

-- gh-3544: concatenation operator accepts only TEXT and BLOB.
--
box.sql.execute("SELECT 'abc' || 1;")
box.sql.execute("SELECT 'abc' || 1.123;")
box.sql.execute("SELECT 1 || 'abc';")
box.sql.execute("SELECT 1.123 || 'abc';")
box.sql.execute("SELECt 'a' || 'b' || 1;")
-- What is more, they must be of the same type.
--
box.sql.execute("SELECT 'abc' || randomblob(5);")
box.sql.execute("SELECT randomblob(5) || 'x';")
-- Result of BLOBs concatenation must be BLOB.
--
box.sql.execute("VALUES (TYPEOF(randomblob(5) || zeroblob(5)));")

-- gh-3954: LIKE accepts only arguments of type TEXT and NULLs.
--
box.sql.execute("CREATE TABLE t1 (s SCALAR PRIMARY KEY);")
box.sql.execute("INSERT INTO t1 VALUES (randomblob(5));")
box.sql.execute("SELECT * FROM t1 WHERE s LIKE 'blob';")
box.sql.execute("SELECT * FROM t1 WHERE 'blob' LIKE s;")
box.sql.execute("SELECT * FROM t1 WHERE 'blob' LIKE x'0000';")
box.sql.execute("SELECT s LIKE NULL FROM t1;")
box.sql.execute("DELETE FROM t1;")
box.sql.execute("INSERT INTO t1 VALUES (1);")
box.sql.execute("SELECT * FROM t1 WHERE s LIKE 'int';")
box.sql.execute("SELECT * FROM t1 WHERE 'int' LIKE 4;")
box.sql.execute("SELECT NULL LIKE s FROM t1;")
box.space.T1:drop()
