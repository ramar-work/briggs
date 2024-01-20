DROP DATABASE IF EXISTS pg_briggs_media_tests;

CREATE DATABASE pg_briggs_media_tests;

\connect pg_briggs_media_tests;

CREATE TABLE postgres_fancy_media (

/* these will be tricky */
/*
smallserialv smallserial,
bigserialv bigserial,
*/
serialv serial,

/* ints */
smallintv smallint,
integerv integer,
bigintv bigint,
decimalv decimal,
numericv numeric,
realv real,
doublev double precision,

/* TEXT equivalent, note the character limits */
varcharv varchar,
charv char,
bpcharv bpchar,
textv text,
booleanv boolean,

/* Date types, nano second precision would be the best bet, but hard */
timestampv timestamp,
datev date,
timev time,
/*intervalv interval,*/

/*
timestampv_wp timestamp,
datev_wp date,
timev_wp time,
*/

/* BLOB equivalent */
blobv bytea

);


/*INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev) VALUES (32000,123,1,1,1,1,234.02,'A','A','A','T','The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','00:00');*/

INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (32000,123,1,1,1,1,234.02,'A','A','A','T','The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','00:00','The quick brown fox jumps over the lazy dog. ☺' );
/*
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (0,0,0,0,0,0,0,'B','B','B','t','The quick brown fox jumps over the lazy dog.','2024-01-01','January 8, 1999','00:00','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','1/8/1999','00:00','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','1/18/1999','00:00','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','01/02/03','00:00','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','1999-Jan-08','00:00','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','Jan-08-1999','00:00','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','08-Jan-1999','00:00','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','08-Jan-99','00:00','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','Jan-08-99','00:00','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','19990108','00:00','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','990108','00:00','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','1999.008','00:00','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','J2451187','00:00','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','January 8, 99 BC','00:00','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','04:05:06.789','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','04:05:06','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','04:05','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','04:05:06','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','04:05 AM','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','04:05 PM','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','04:05:06.789-8','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','04:05:06-08:00','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','04:05-08:00','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','040506-08','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','040506+0730','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','040506+07:30:00','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','04:05:06 PST','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','2024-01-01','1999-01-08','2003-04-12 04:05:06 America/New_York','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','1999-01-08 04:05:06','2024-01-01','04:05','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','1999-01-08 04:05:06 -8:00','2024-01-01','04:05','The quick brown fox jumps over the lazy dog. ☺' );
INSERT INTO postgres_fancy_media (smallintv,integerv,bigintv,decimalv,numericv,realv,doublev,varcharv,charv,bpcharv,booleanv,textv,timestampv,datev,timev,blobv) VALUES (-19324,2147483647,1,1,1,1,234234.232342345,'C','C','C','F','The quick brown fox jumps over the lazy dog.','January 8 04:05:06 1999 PST','2024-01-01','04:05','The quick brown fox jumps over the lazy dog. ☺' );
*/
