SELECT name
FROM (SELECT hikeno, SUM(distance) AS hdist
      FROM HIKEPTS GROUP BY hikeno) AS A, HIKE
WHERE A.hikeno=HIKE.hikeno AND HIKE.location LIKE "%s" AND hdist=(
    SELECT MIN(h2)
    FROM (
        SELECT hikeno, SUM(distance) AS h2
        FROM HIKEPTS
        GROUP BY hikeno
         ) AS B);
SELECT COUNT(hikeno) AS "Number of Hikes"
FROM (
    SELECT hikeno, COUNT(leg) AS nlegs
    FROM HIKEPTS GROUP BY hikeno
    ) AS A
WHERE (A.nlegs > %s);
SELECT name
FROM (
    SELECT fileno, ptno, RADIANS(lat) AS flat, RADIANS(lon) AS flon
    FROM WAYPTS
     ) AS A, HIKEPTS, HIKE
WHERE HIKEPTS.ptno=A.ptno AND HIKEPTS.fileno=A.fileno AND leg=0 AND HIKEPTS.hikeno=HIKE.hikeno AND
ATAN2(
    SQRT(
        POW( COS(flat) * SIN(%(slon)f - flon), 2) +
        POW( COS(%(slat)f) * SIN(flat) - SIN(%(slat)f) * COS(flat) * COS(%(slon)f - flon), 2)),
    SIN(%(slat)f) * SIN(flat) +
    COS(%(slat)f) * COS(flat) * COS(%(slon)f - flon)) * 6371.01 < %(dist)f;

SELECT hike

;
%s
