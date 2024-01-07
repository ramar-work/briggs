CREATE DATABASE pg_briggs_media_tests;

CREATE TABLE postgres_fancy_media (

/* these will be tricky */
/*
smallserialv smallserial,
serialv serial,
bigserialv bigserial,
*/

/* ints */
smallintv smallint,
integerv integer,
bigintv bigint,
decimalv decimal,
numericv numeric,
realv real,
doublev double,

/* TEXT equivalent, note the character limits */
varcharv varchar,
charv char,
bpcharv bpchar,
textv text,

/* Date types, nano second precision would be the best bet, but hard */
timestampv timestamp,
datev date,
timev time,
intervalv interval,

/* BLOB equivalent */
byteav bytea

);
