CREATE TABLE IF NOT EXISTS HIKE (
    hikeno      INT AUTO_INCREMENT,
    name        VARCHAR(30),
    location    VARCHAR(30),
    comment     VARCHAR(80),
    rating      SMALLINT,
    note        TINYTEXT,
    PRIMARY KEY (hikeno)
)
CREATE TABLE IF NOT EXISTS WAYPTS (
    fileno      INT,
    ptno        INT,
    id          VARCHAR(30),
    comment     VARCHAR(80),
    lat         DOUBLE,
    lon         DOUBLE,
    PRIMARY KEY (fileno, ptno)
)
CREATE TABLE IF NOT EXISTS HIKEPTS (
    hikeno      INT,
    fileno      INT,
    ptno        INT,
    leg         INT,
    distance    FLOAT,
    PRIMARY KEY (hikeno, leg),
    FOREIGN KEY (hikeno) REFERENCES HIKE,
    FOREIGN KEY (fileno, ptno) REFERENCES WAYPTS
)
