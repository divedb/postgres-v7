CREATE TABLE STUDENT (
    sno   INT PRIMARY KEY,
    sname VARCHAR(10),
    ssex  INT
);

CREATE TABLE COURSE (
    cno   INT PRIMARY KEY,
    cname VARCHAR(10),
    tno   INT
);

CREATE TABLE SCORE (
    sno    INT,
    cno    INT,
    degree INT
);

CREATE TABLE TEACHER (
    tno   INT PRIMARY KEY,
    tname VARCHAR(10),
    tsex  INT
);

CREATE TABLE test_a (
    a INT,
    b INT,
    c INT,
    d INT
);

CREATE TABLE test_b (
    a INT,
    b INT,
    c INT,
    d INT
);

CREATE TABLE test_c (
    a INT,
    b INT,
    c INT,
    d INT
);

CREATE TABLE test_d (
    a INT,
    b INT,
    c INT,
    d INT
);

EXPLAIN SELECT * FROM test_a WHERE EXISTS (
    SELECT a FROM test_b WHERE test_a.a = test_b.a
);

EXPLAIN SELECT * FROM test_a WHERE EXISTS (
    SELECT a FROM test_b
);

EXPLAIN SELECT * FROM test_a WHERE EXISTS (
    SELECT SUM(a) FROM test_b
    WHERE test_a.a = test_b.a
);

EXPLAIN SELECT * FROM test_a WHERE a > ANY (SELECT a FROM test_b);

-- from子句中各个表结构
SELECT * FROM student LEFT JOIN score ON TRUE,
(SELECT * FROM teacher) AS t, course, (VALUES(1,1)) AS num(x, y),
GENERATE_SERIES(1, 10) AS gs(z);

-- ANY类型子连接提升测试语句
SELECT sname FROM student WHERE sno > ANY(SELECT sno FROM score);