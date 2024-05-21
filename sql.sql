-- .mode csv
-- .import C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv test

-- SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '@');
-- SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '.') - 1;
--SELECT SUBSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', (SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '@') + 1), ((SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '.')) - (SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '@') + 1))); 
--SELECT INSTR((SELECT SUBSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', (SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '@') + 1), ((SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '.')) - (SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '@') + 1)))), '-');


------------------------------------EXTRACTING THE EXAMPLE SUBSTRING CONTINGENCY ONLY------------------------------------
--SELECT SUBSTR((SELECT SUBSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', (SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '@') + 1), ((SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '.')) - (SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '@') + 1)))), 1, 
--(SELECT INSTR((SELECT SUBSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', (SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '@') + 1), ((SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '.')) - (SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '@') + 1)))), '-')) - 1);


-- SELECT CASE WHEN 
-- ((SELECT SUBSTR('brndgens', '#')) = 0) 
-- THEN (INSERT INTO contigency (`contingency name`) VALUES ('hello'));
-- ELSE 11 END AS result;

--INSERT INTO contingency (`contingency name`) VALUES ('hello') WHERE ((SELECT SUBSTR('brndgens', '#')) = 0);


------------------------------------INSERTING THE EXAMPLE INTO THE TABLE------------------------------------
-- INSERT INTO contingency (`contingency name`) VALUES ((SELECT SUBSTR((SELECT SUBSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', (SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '@') + 1), ((SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '.')) - (SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '@') + 1)))), 1, 
-- (SELECT INSTR((SELECT SUBSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', (SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '@') + 1), ((SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '.')) - (SELECT INSTR('C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv', '@') + 1)))), '-')) - 1)));

------------------------------------CREATE CONTINGENCY TABLE------------------------------------
-- CREATE TABLE contingency (
--     `contingency name` text PRIMARY KEY NOT NULL,
--     `NERC category` text 
-- );

--SELECT * FROM contingency

SELECT 5*6;