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

DROP DATABASE IF EXISTS mysql_briggs_media_tests;

CREATE DATABASE mysql_briggs_media_tests;

USE mysql_briggs_media_tests;

CREATE TABLE mysql_fancy_media (
	smallintv SMALLINT,
	integerv INT,
	bigintv BIGINT,
	decimalv DECIMAL,
	numericv NUMERIC,
	realv FLOAT,
	floatv FLOAT,
	doublev DOUBLE,
	charv CHAR,
	varcharv VARCHAR(1),
	varchar_bigv VARCHAR(64),
	booleanv BOOLEAN,
	textv TEXT,
	blobv BLOB,
	timestampv TIMESTAMP,
	datev DATE,
	timev TIME,
	datetimev DATETIME,
	yearv YEAR
);

INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (32000,123,1,1,1,1,234.02,234.02,'A','The quick brown fox jumps over the lazy dog.','A',1,'The quick brown fox jumps over the lazy dog.','1970-01-19 03:14:07.500','1999-01-08','11:12','1000-01-01 00:00:00.000000',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (0,0,0,0,0,0,0,0,'B','The quick brown fox jumps over the lazy dog.','B',1,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','11:12','2024-01-01 12:00:01','2000','The quick brown fox jumps over the lazy dog. ☺' );
/*
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','11:12','2021-12-31 23:59:59.49',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','11:12','2024-01-01 12:00:01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','11:12','2024-01-01 12:00:01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','11:12','2024-01-01 12:00:01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','11:12','2024-01-01 12:00:01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','11:12','2024-01-01 12:00:01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','11:12','2024-01-01 12:00:01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','838:59:59','2024-01-01 12:00:01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','19990108','838:59:59','2024-01-01 12:00:01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','990108','838:59:59','2024-01-01 12:00:01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','19990108','838:59:59','2024-01-01 12:00:01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','19990108','838:59:59','2024-01-01 12:00:01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','19990108','838:59:59','2024-01-01 12:00:01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','04:05:06.789','2024-01-01 12:00:01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','04:05:06','2024-01-01 12:00:01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','04:05','2024-01-01 12:00:01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','04:05:06','2024-01-01 12:00:01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','23:01','2024-01-01 12:00:01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','23:01','9999-12-31',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','838:59:59','2024-01-01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','838:59:59','2024-01-01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','838:59:59','2024-01-01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','838:59:59','2024-01-01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','838:59:59','2024-01-01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','838:59:59','2024-01-01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','04:05:06','2024-01-01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','04:05','2024-01-01',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08 04:05:06','04:05','1999-01-08 04:05:06',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08 04:05:06','04:05','1999-01-08 04:05:06',2024,'The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO mysql_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,floatv,doublev,varcharv,varchar_bigv,charv,booleanv,textv,timestampv,datev,timev,datetimev,yearv,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,234234.232342345,'C','The quick brown fox jumps over the lazy dog.','C',0,'The quick brown fox jumps over the lazy dog.','2037-01-19 03:14:07.500','1999-01-08 04:05:06','04:05','1999-01-08 04:05:06',2024,'The quick brown fox jumps over the lazy dog. ☺' );
*/


SELECT COUNT(*) FROM mysql_fancy_media;
