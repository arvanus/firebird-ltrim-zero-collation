/*
 * Test script for LTRIM_ZERO Collation
 * Run this script after installing the collation
 */

-- Configure to see results
SET HEADING ON;
SET NAMES WIN1252;

-- ===========================================================================
-- PART 1: Create the collation in the database
-- ===========================================================================

SHOW COLLATION;

-- Create collation for WIN1252
CREATE COLLATION WIN1252_LTRIM_ZERO_AI
    FOR WIN1252
    FROM EXTERNAL ('WIN1252_LTRIM_ZERO');

-- Verify it was created
SELECT RDB$COLLATION_NAME, RDB$CHARACTER_SET_ID, RDB$COLLATION_ID
FROM RDB$COLLATIONS
WHERE RDB$COLLATION_NAME CONTAINING 'LTRIM';

-- ===========================================================================
-- PART 2: Basic comparison tests
-- ===========================================================================

SELECT '----------- TEST 1: Basic Comparisons -----------' FROM RDB$DATABASE;

-- All should return TRUE (1)
SELECT '00001' COLLATE WIN1252_LTRIM_ZERO_AI = '1' AS "00001=1",
       '0A' COLLATE WIN1252_LTRIM_ZERO_AI = 'A' AS "0A=A",
       '   A' COLLATE WIN1252_LTRIM_ZERO_AI = 'A' AS "SPC_A=A",
       'a' COLLATE WIN1252_LTRIM_ZERO_AI = 'A' AS "a=A"
FROM RDB$DATABASE;

-- Test with different amounts of zeros
SELECT '000000001' COLLATE WIN1252_LTRIM_ZERO_AI = '1' AS "MANY_ZEROS",
       '0' COLLATE WIN1252_LTRIM_ZERO_AI = '0' AS "ZERO=ZERO",
       '00000' COLLATE WIN1252_LTRIM_ZERO_AI = '0' AS "ZEROS=ZERO"
FROM RDB$DATABASE;

-- ===========================================================================
-- PART 3: Create test table
-- ===========================================================================

SELECT '----------- TEST 2: Test Table -----------' FROM RDB$DATABASE;

-- Create domain with the collation
CREATE DOMAIN CODE_LTRIM AS VARCHAR(20)
    CHARACTER SET WIN1252
    COLLATE WIN1252_LTRIM_ZERO_AI;

-- Create table
CREATE TABLE TEST_LTRIM (
    ID INTEGER,
    CODE CODE_LTRIM,
    DESCRIPTION VARCHAR(100)
);

-- Insert test data
INSERT INTO TEST_LTRIM VALUES (1, '00001', 'Code with leading zeros');
INSERT INTO TEST_LTRIM VALUES (2, '0001', 'Code with 3 zeros');
INSERT INTO TEST_LTRIM VALUES (3, '001', 'Code with 2 zeros');
INSERT INTO TEST_LTRIM VALUES (4, '01', 'Code with 1 zero');
INSERT INTO TEST_LTRIM VALUES (5, '1', 'Code without zeros');
INSERT INTO TEST_LTRIM VALUES (6, '   1', 'Code with spaces');
INSERT INTO TEST_LTRIM VALUES (7, '00000A', 'Letter A with zeros');
INSERT INTO TEST_LTRIM VALUES (8, 'A', 'Letter A simple');
INSERT INTO TEST_LTRIM VALUES (9, 'a', 'Letter a lowercase');
INSERT INTO TEST_LTRIM VALUES (10, '00000B', 'Letter B with zeros');
INSERT INTO TEST_LTRIM VALUES (11, 'B', 'Letter B simple');

COMMIT;

-- ===========================================================================
-- PART 4: Search tests
-- ===========================================================================

SELECT '----------- TEST 3: Searches -----------' FROM RDB$DATABASE;

-- Search for '1' - should find IDs 1-6
SELECT ID, CODE, DESCRIPTION
FROM TEST_LTRIM
WHERE CODE = '1'
ORDER BY ID;

-- Search for 'A' - should find IDs 7-9
SELECT ID, CODE, DESCRIPTION
FROM TEST_LTRIM
WHERE CODE = 'A'
ORDER BY ID;

-- ===========================================================================
-- PART 5: Sorting tests
-- ===========================================================================

SELECT '----------- TEST 4: Sorting -----------' FROM RDB$DATABASE;

-- Sort - should group equivalent values
SELECT CODE, COUNT(*) AS QTY
FROM TEST_LTRIM
GROUP BY CODE
ORDER BY CODE;

-- Full sort
SELECT ID, CODE, DESCRIPTION
FROM TEST_LTRIM
ORDER BY CODE, ID;

-- ===========================================================================
-- PART 6: Index test
-- ===========================================================================

SELECT '----------- TEST 5: Index -----------' FROM RDB$DATABASE;

-- Create index
CREATE INDEX IDX_CODE ON TEST_LTRIM(CODE);
COMMIT;

-- Check execution plan - should use the index
SET PLAN ON;
SELECT ID, CODE
FROM TEST_LTRIM
WHERE CODE = '00001';
SET PLAN OFF;

-- ===========================================================================
-- PART 7: JOIN tests
-- ===========================================================================

SELECT '----------- TEST 6: JOIN -----------' FROM RDB$DATABASE;

-- Self-join to find duplicates
SELECT t1.ID AS ID1, t1.CODE AS CODE1,
       t2.ID AS ID2, t2.CODE AS CODE2
FROM TEST_LTRIM t1
JOIN TEST_LTRIM t2 ON t1.CODE = t2.CODE
WHERE t1.ID < t2.ID
ORDER BY t1.CODE, t1.ID;

-- ===========================================================================
-- PART 8: DISTINCT test
-- ===========================================================================

SELECT '----------- TEST 7: DISTINCT -----------' FROM RDB$DATABASE;

-- Unique values - should return only '1', 'A', 'B'
SELECT DISTINCT CODE
FROM TEST_LTRIM
ORDER BY CODE;

-- ===========================================================================
-- PART 9: NULL test
-- ===========================================================================

SELECT '----------- TEST 8: NULL -----------' FROM RDB$DATABASE;

INSERT INTO TEST_LTRIM VALUES (100, NULL, 'NULL code');
COMMIT;

SELECT ID, CODE, DESCRIPTION
FROM TEST_LTRIM
WHERE CODE IS NULL;

-- ===========================================================================
-- PART 10: Empty strings and zeros-only test
-- ===========================================================================

SELECT '----------- TEST 9: Special Cases -----------' FROM RDB$DATABASE;

INSERT INTO TEST_LTRIM VALUES (200, '', 'Empty string');
INSERT INTO TEST_LTRIM VALUES (201, '0', 'Single zero');
INSERT INTO TEST_LTRIM VALUES (202, '00', 'Two zeros');
INSERT INTO TEST_LTRIM VALUES (203, '000', 'Three zeros');
INSERT INTO TEST_LTRIM VALUES (204, '     ', 'Only spaces');
COMMIT;

-- Should group zeros together
SELECT CODE, COUNT(*) AS QTY
FROM TEST_LTRIM
WHERE CODE IS NOT NULL AND CODE <> ''
GROUP BY CODE
HAVING COUNT(*) > 1
ORDER BY CODE;

-- ===========================================================================
-- PART 11: Performance comparison (optional)
-- ===========================================================================

SELECT '----------- TEST 10: Performance -----------' FROM RDB$DATABASE;

-- Insert more data for performance testing
set term ^ ;
EXECUTE BLOCK
AS
DECLARE I INTEGER;
BEGIN
  I = 1000;
  WHILE (I < 2000) DO
  BEGIN
    INSERT INTO TEST_LTRIM VALUES (:I, LPAD(:I, 10, '0'), 'Test ' || :I);
    I = I + 1;
  END
END^
set term ; ^
COMMIT;

-- Check count
SELECT COUNT(*) AS TOTAL_RECORDS FROM TEST_LTRIM;

-- Search with index
SET PLAN ON;
SET STATS ON;
SELECT COUNT(*) FROM TEST_LTRIM WHERE CODE = '00001500';
SET STATS OFF;
SET PLAN OFF;

-- ===========================================================================
-- EXPECTED RESULTS
-- ===========================================================================

SELECT '----------- EXPECTED RESULTS -----------' FROM RDB$DATABASE;

SELECT 'TEST 1: All comparisons = TRUE (1)' AS RESULT FROM RDB$DATABASE
UNION ALL
SELECT 'TEST 2: Table created successfully' FROM RDB$DATABASE
UNION ALL
SELECT 'TEST 3: Search "1" finds 6 records' FROM RDB$DATABASE
UNION ALL
SELECT 'TEST 4: Sorting groups equivalent values' FROM RDB$DATABASE
UNION ALL
SELECT 'TEST 5: Index created and used in searches' FROM RDB$DATABASE
UNION ALL
SELECT 'TEST 6: JOIN finds duplicates' FROM RDB$DATABASE
UNION ALL
SELECT 'TEST 7: DISTINCT returns 3 values (1, A, B)' FROM RDB$DATABASE
UNION ALL
SELECT 'TEST 8: NULL works normally' FROM RDB$DATABASE
UNION ALL
SELECT 'TEST 9: Zeros and spaces handled correctly' FROM RDB$DATABASE
UNION ALL
SELECT 'TEST 10: Performance with index acceptable' FROM RDB$DATABASE;

-- ===========================================================================
-- CLEANUP (optional - comment out if you want to keep the data)
-- ===========================================================================

/*
SELECT '----------- CLEANUP -----------' FROM RDB$DATABASE;

DROP TABLE TEST_LTRIM;
DROP DOMAIN CODE_LTRIM;
DROP COLLATION WIN1252_LTRIM_ZERO_AI;
COMMIT;

SELECT 'Cleanup completed!' FROM RDB$DATABASE;
*/
