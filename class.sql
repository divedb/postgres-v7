CREATE TABLE grade (
    gno    VARCHAR(20) PRIMARY KEY
);

CREATE TABLE class (
    classno   VARCHAR(20) PRIMARY KEY,
    classname VARCHAR(30),
    gno       VARCHAR(20),
    FOREIGN KEY (gno) REFERENCES grade(gno)
);

CREATE TABLE student (
    sno     VARCHAR(20) PRIMARY KEY,
    sname   VARCHAR(30),
    sex     VARCHAR(5),
    age     INT,
    nation  VARCHAR(20),
    classno VARCHAR(20),
    FOREIGN KEY(classno) REFERENCES class(classno)
);

CREATE TABLE course (
    cno         VARCHAR(20) PRIMARY KEY,
    cname       VARCHAR(30),
    credit      INT,
    priorcourse VARCHAR(20),
    FOREIGN KEY (priorcourse) REFERENCES course(cno)
);

CREATE TABLE sc (
    sno     VARCHAR(20),
    cno     VARCHAR(20),
    score   INT,
    PRIMARY KEY (sno, cno),
    FOREIGN KEY (sno) REFERENCES student(sno),
    FOREIGN KEY (cno) REFERENCES course(cno)
);

-- 测试语句1
SELECT classno, classname, AVG(score) AS avg_score
FROM sc, (SELECT * FROM class WHERE class.gno = '2005') AS sub
WHERE
    sc.sno IN (SELECT sno FROM student WHERE student.classno=sub.classno)
AND
    sc.cno IN (SELECT cno FROM course WHERE cname = '高等数学')
GROUP BY classno, classname
HAVING AVG(score) > 80.0
ORDER BY avg_score;