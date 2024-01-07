/* 
Per Mysql Docs:

To facilitate the use of code written for SQL implementations from other vendors, MySQL maps data types as shown in the following table. These mappings make it easier to import table definitions from other database systems into MySQL.

Other Vendor Type	MySQL Type
BOOL	TINYINT
BOOLEAN	TINYINT
CHARACTER VARYING(M)	VARCHAR(M)
FIXED	DECIMAL
FLOAT4	FLOAT
FLOAT8	DOUBLE
INT1	TINYINT
INT2	SMALLINT
INT3	MEDIUMINT
INT4	INT
INT8	BIGINT
LONG VARBINARY	MEDIUMBLOB
LONG VARCHAR	MEDIUMTEXT
LONG	MEDIUMTEXT
MIDDLEINT	MEDIUMINT
NUMERIC	DECIMAL

Date conversion:
Data Type	“Zero” Value
DATE	'0000-00-00'
TIME	'00:00:00'
DATETIME	'0000-00-00 00:00:00'
TIMESTAMP	'0000-00-00 00:00:00'
YEAR	0000

*/

CREATE DATABASE mysql_briggs_media_tests;

CREATE TABLE mysql_fancy_media (

/* All integer values */
integerv INTEGER,
smallintv SMALLINT,
tinyintv TINYINT,
mediumintv MEDIUMINT,
bigintv BIGINT,
decimalv DECIMAL,
numericv NUMERIC,
floatv FLOAT,
doublev DOUBLE,
/*bitv BIT,*/

/* Date time values - Use GMT format of date as unix timestamp for simple conversion */
datev DATE,
timev TIME,
datetimev DATETIME,
timestampv TIMESTAMP,
yearv YEAR,

/* String and character and blob data types */
charv CHAR,
varcharv VARCHAR,

/* TEXT maps to any of these: TINYTEXT, TEXT, MEDIUMTEXT, LONGTEXT */
tinytextv TINYTEXT,
textv TEXT,
mediumtextv MEDIUMTEXT,
longtextv LONGTEXT,

binaryv BINARY,
varbinaryv VARBINARY,
/* BLOBs map to any of these: TINYBLOB, BLOB, MEDIUMBLOB, LONGBLOB */
tinyblobv TINYBLOB,
blobv BLOB,
mediumblobv MEDIUMBLOB,
longblobv LONGBLOB,
/* Supporting these is difficult and probably not a good use of time */
/* ENUM */
/* SET */

);
