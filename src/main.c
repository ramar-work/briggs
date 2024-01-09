/* ------------------------------------------------------- *
 * briggs.c 
 * ========
 * A tool for converting CSV sheets into other data formats. 
 *
 * Usage
 * -----
 * briggs can be used to turn CSV data into a SQL upload, a
 * C-style struct, JSON, XML and more. 
 *
 * Options are as follows:
 *
 * 
 * TODO
 * ----
 * - Handle unsigned characters, then it's easy to digest xls[x], etc
 * - Handle Unicode (uint32_t?)
 * - Give user the ability to generate values:
 * 	one flag ought to be used to generate some things
 * 	if I give it a file, use a single line
 * 	if I give it multiple values, use those
 * 	if I give it a string, then do yet something else...
 * 	briggs -t range=[ x, y, z ] 
 * - Start testing on Windows
 * - Add ability to generate random test data
 * 	- Numbers
 * 	- Zips
 * 	- Addresses
 * 	- Names
 * 	- Words
 * 	- Paragraphs
 * 	- URLs
 * 	- Images (probably from a directory is the most useful)
 * 	
 * ------------------------------------------------------- */
#define _POSIX_C_SOURCE 200809L
#define VERSION "0.01"
#define TAB "\t\t\t\t\t\t\t\t\t"
#define ERRLEN 2048

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include "../vendor/zwalker.h"
#include "../vendor/util.h"

/* Can compile this to be bigger if need be */
#define MAX_STMT_SIZE 2048

/* Optionally include support for MySQL */
#ifdef BMYSQL_H
 #include <mysql/mysql.h>
#endif

/* Optionally include support for Postgres */
#ifdef BPGSQL_H
 #include <libpq-fe.h>
#endif

/* Optionally include support for SQLite3 */
#ifdef BSQLITE_H
 #error "SQLite support is incomplete.  Do not enable it yet.  For your own good. Thanks."
 #include <sqlite3.h>
#endif


#define nerr( ... ) \
	(fprintf( stderr, "briggs: " ) ? 1 : 1 ) && (fprintf( stderr, __VA_ARGS__ ) ? 0 : 0 )

#define PERR( ... ) \
	(fprintf( stderr, "briggs: " ) ? 1 : 1 ) && (fprintf( stderr, __VA_ARGS__ ) ? 0 : 0 )

#define SHOW_COMPILE_DATE() \
	fprintf( stderr, "briggs v" VERSION " compiled: " __DATE__ ", " __TIME__ "\n" )

#ifdef WIN32
 #define NEWLINE "\r\n"
#else
 #define NEWLINE "\n"
#endif

#ifdef DEBUG_H
 #define DPRINTF( ... ) fprintf( stderr, __VA_ARGS__ )
 #define N(A) #A
#else
 #define DPRINTF( ... ) 0
 #define N(A) "" 
#endif

/* Doing an index here will return the right type from array */
#define CHECK_SQL_TYPE( type, type_array ) \
	memchr( (unsigned char *)type_array, type, sizeof( type_array ) )

/* Get is safer */
#define GET_SQL_TYPE( type, type_array ) \
	memchr( (unsigned char *)type_array, type, sizeof( type_array ) )

#define STRLCMP(H,S) \
	( strlen( H ) >= sizeof( S ) - 1 ) && !memcmp( H, S, sizeof( S ) - 1 )
	

/**
 * stream_t
 *
 * A stream type or format to convert the records into.
 *
 */
typedef enum {
  STREAM_NONE = -1
, STREAM_PRINTF = 0
, STREAM_JSON
, STREAM_XML
, STREAM_SQL
, STREAM_CSTRUCT
, STREAM_COMMA
, STREAM_CARRAY
, STREAM_JCLASS
, STREAM_CUSTOM
#ifdef BSQLITE_H
, STREAM_SQLITE
#endif
#ifdef BPGSQL_H
, STREAM_PGSQL
#endif
#ifdef BMYSQL_H
, STREAM_MYSQL
#endif
} stream_t;



/**
 * streamtype_t
 *
 * Strings that match up to stream types
 *
 */
typedef struct streamtype_t {
	char *type;
	stream_t streamfmt;
} streamtype_t;


streamtype_t streams[] = { 
#if 0
  { "none", STREAM_NONE }
, { "none", STREAM_PRINTF }
#endif
  { "json", STREAM_JSON }
, { "xml", STREAM_XML }
, { "sql", STREAM_SQL }
, { "struct", STREAM_CSTRUCT }
, { "csv", STREAM_COMMA }
, { "array", STREAM_CARRAY }
, { "class", STREAM_JCLASS }
#ifdef BSQLITE_H
, { "sqlite3", STREAM_SQLITE }
#endif
#ifdef BPGSQL_H
, { "postgres", STREAM_PGSQL }
#endif
#ifdef BMYSQL_H
, { "mysql", STREAM_MYSQL }
#endif
#if 0
, { "none", STREAM_CUSTOM }
#endif
, { NULL, 0 }
};

/**
 * case_t 
 *
 * Case choice for output formats. 
 *
 */
typedef enum case_t {
	NONE = 0,
	CASE_SNAKE,
	CASE_CAMEL
} case_t;



/**
 * type_t 
 *
 * Choose a general type when reading from or writing to a source.
 *
 */
typedef enum type_t {
	T_NULL = 0,
	T_CHAR,
	T_STRING,
	T_INTEGER,
	T_DOUBLE,
	T_BOOLEAN,
	T_BINARY, /* Assume that unsigned char * is what is expected */
	// T_DATE, - Conversion to a base type (via tv_sec, tv_nsec ) might make the most sense)
	/* T_BINARY // If integrating this, consider using a converter */
} type_t;


static const char * itypes[] = {
	[T_NULL] = "T_NULL",
	[T_CHAR] = "T_CHAR",
	[T_STRING] = "T_STRING",
	[T_INTEGER] = "T_INTEGER",
	[T_DOUBLE] = "T_DOUBLE",
	[T_BOOLEAN] = "T_BOOLEAN",
	[T_BINARY] = "T_BINARY",
};



#if 0

static char *cstr_type ( type_t type ) {
	return c_family_typenames[ (int)type ];	
}

static char *x_typenames[] = {
	"null",
	"char",
	"String",
	"int",
	"double",
	"bool"
};


static char *c_family_typenames[] = {
	"null",
	"char",
	"char *",
	"int",
	"double",
	"bool"
};
#endif


/**
 * sqldef_t 
 *
 * ? 
 *
 */
typedef struct sqldef_t {
	const char *id_fmt;
	const char *coldefs[8];
} sqldef_t;



/**
 * sqltype_t 
 *
 * Choices of SQL backend
 *
 */
typedef enum sqltype_t {
	SQL_NONE,
	SQL_SQLITE3,
#ifdef BMYSQL_H
	SQL_MYSQL,
#endif
#ifdef BPGSQL_H
	SQL_POSTGRES
#endif
#if 0
	SQL_ORACLE,
	SQL_MSSQLSRV,
#endif
} sqltype_t;



/**
 * dbtype_t 
 *
 * Choose a database type 
 *
 */
typedef enum {
	DB_NONE,
	DB_FILE,
	DB_SQLITE,
#ifdef BMYSQL_H
	DB_MYSQL,
#endif
#ifdef BPGSQL_H
	DB_POSTGRESQL
#endif
#if 0
	SQL_ORACLE,
	SQL_MSSQLSRV,
#endif
} dbtype_t;

/**
 * typemap_t 
 *
 * Map native SQL types to briggs types and more.
 *
 */
typedef struct typemap_t {
	int ntype; /* best I can do ... */
	const char *libtypename;
	const char *typename;
	type_t basetype;	
	char preferred;	
} typemap_t;



/**
 * header_t 
 *
 * General format for headers or column names coming from a datasource
 *
 */
typedef struct header_t {
	/* Name of the column */
	char label[ 128 ];

	/* A sensible base type */
	type_t type; 

	/* Whether or not this column should be treated as a primary key */
	int primary;

	/* Options that may have come from a database engine */
	int options;

	/* The native type used to save this value in the original datasource */
	int ntype;

	/* A coerced type (preferably a name) */
	typemap_t *ctype;
} header_t;



/**
 * record_t
 *
 * General format for values coming from a datasource
 *
 */
typedef struct record_t { 
  char *k; 
  unsigned char *v; 
	type_t type;
	type_t exptype;
} record_t;


typedef struct dbengine_t {
int i;
}  dbengine_t;

/**
 * column_t
 *
 * General format for values coming from a datasource
 *
 */
typedef struct column_t { 
  char *k; 
  unsigned char *v; 
	type_t type;
	type_t exptype;
	int realtype;
	unsigned int len;
} column_t;



/**
 * row_t
 *
 * General format for values coming from a datasource
 *
 */
typedef struct row_t { 
	column_t **columns;
	int clen;
#if 0
	// The database engines have some strange requirements
#endif
} row_t;




/**
 * dsn_t 
 *
 * Define how to handle general connections to a database 
 *
 */
typedef struct dsn_t {
	char username[ 64 ];
	char password[ 64 ];
	char hostname[ 256 ];
	char dbname[ 64 ];
	char tablename[ 64 ];
	char connoptions[ 1028 ];
	dbtype_t type;
	int port;
	int options;	
	//char socketpath[ 2048 ];
	char *connstr;
	//FILE *file;
	void *conn;
	void *res;
	header_t **headers;
	row_t **rows;
	int hlen;
	int rlen;
	int clen;
	char **defs;	
	FILE *output;
	const typemap_t *typemap;	
	const typemap_t *convmap;	
} dsn_t;



/**
 * function_t 
 *
 * ...
 *
 */
typedef struct function_t {
	const char *type;
  const char *left_fmt;
  const char *right_fmt;
  int i;
  void (*execute)( int, column_t * ); 
  void (*open)( dsn_t * ); 
  void (*close)( dsn_t * ); 
  void *p;
} function_t;



#define TYPEMAP_TERM INT_MIN


// This is for default types.  A map IS a lot of overhead...
const typemap_t default_map[] = {
	{ T_NULL, N(T_NULL), NULL, T_NULL, 0 },
	{ T_STRING, N(T_STRING), "TEXT", T_STRING, 1 },
	/* NOTE: This will force STRING to take precedence over CHAR.  DO NOT TOUCH THIS */
	{ T_CHAR, N(T_CHAR), "TEXT", T_CHAR, 1 },
	{ T_INTEGER, N(T_INTEGER), "INTEGER", T_INTEGER, 1 },
	{ T_DOUBLE, N(T_DOUBLE), "REAL", T_DOUBLE, 1 },
	{ T_BINARY, N(T_BINARY), "BLOB", T_BINARY, 1 },
	//{ T_BOOLEAN, N(T_BOOLEAN), "BOOLEAN", T_BOOLEAN },
	{ TYPEMAP_TERM },
};


static const char * dbtypes[] = {
	[DB_NONE] = "none",
	[DB_FILE] = "file",
	[DB_SQLITE] = "SQLite3",
	[DB_MYSQL] = "MySQL",
	[DB_POSTGRESQL] = "Postgres",
};


static const char * get_conn_type( dbtype_t type ) {
	if ( type < sizeof( dbtypes ) / sizeof( const char * ) && dbtypes[ type ] ) {
		return dbtypes[ type ];
	}
	return NULL;
}


typemap_t * get_typemap ( const typemap_t *types, int type ) {
	for ( const typemap_t *t = types; t->ntype != TYPEMAP_TERM; t++ ) {
		if ( t->ntype == type ) return (typemap_t *)t;
	}
	return NULL;
}


typemap_t * get_typemap_by_native_name ( const typemap_t *types, const char *name ) {
	for ( const typemap_t *t = types; t->ntype != TYPEMAP_TERM; t++ ) {
		if ( !strcmp( name, t->typename ) ) {
			return (typemap_t *)t;
		}
	}
	return NULL;
}

typemap_t * get_typemap_by_btype ( const typemap_t *types, int btype ) {
	for ( const typemap_t *t = types; t->ntype != TYPEMAP_TERM; t++ ) {
	#ifdef DEBUG_H
		//fprintf( stderr, "TYPESTUFF %s %s %d != %d\n", t->typename, t->libtypename, t->basetype, btype );
	#endif
		if ( btype == t->basetype && t->preferred ) return (typemap_t *)t;
	}
	return NULL;
}

/* TODO: This is pretty bad, but I don't see a great way to lay these out right now */
/* Postgres specific support */
#ifdef BPGSQL_H

/* TODO: Find a way to get these from the Postgres headers */
#define PG_BOOLOID 16
#define PG_BYTEAOID 17
#define PG_TEXTOID 25 
#define PG_BPCHAROID 1042 
#define PG_VARCHAROID 1043 
#define PG_DATEOID 1082 
#define PG_TIMEOID 1083 
#define PG_TIMESTAMPOID 1114 
#define PG_TIMESTAMPZOID 1184
#define PG_TIMETZOID 1266 
#define PG_INT8OID 20 
#define PG_INT2OID 21
#define PG_INT4OID 23
#define PG_OIDOID 26 
#define PG_NUMERICOID 1700
#define PG_FLOAT4OID 700 
#define PG_FLOAT8OID 701 
#define PG_BITOID 1560 
#define PG_VARBITOID 1562 

// TODO: This is the preferred way to layout this structure,  
static const char * _pgsql_string_map[] = {
	[0] = NULL,
#if 0
	[PG_BOOLOID] = "bool",
#endif
	[PG_BYTEAOID] = "BYTEAOID",
	[PG_TEXTOID] = "TEXTOID",
	[PG_BPCHAROID] = "BPCHAROID",
	[PG_VARCHAROID] = "VARCHAROID",
	[PG_INT2OID] = "INT2OID",
	[PG_INT4OID] = "INT4OID",
	[PG_INT8OID] = "INT8OID",
	[PG_NUMERICOID] = "NUMERICOID",
	[PG_FLOAT4OID] = "FLOAT4OID",
	[PG_FLOAT8OID] = "FLOAT8OID",
#if 0
	[PG_DATEOID] = "PG_DATEOID",
	[PG_TIMEOID] = "PG_TIMEOID",
	[PG_TIMESTAMPOID] = "PG_TIMESTAMPOID",
	[PG_TIMESTAMPZOID] = "PG_TIMESTAMPZOID",
	[PG_TIMETZOID] = "PG_TIMETZOID",
	[PG_OIDOID] = "PG_OIDOID",
	[PG_BITOID] = "PG_BITOID",
	[PG_VARBITOID] = "PG_VARBITOID",
#endif
};


static const typemap_t pgsql_map[] = {
#if 0
	{ TYPENAME(PG_BOOLOID), "bool", T_BOOLEAN },
#endif
	{ PG_INT2OID, N(PG_INT2OID), "smallint", T_INTEGER, 0 },
	{ PG_INT4OID, N(PG_INT4OID), "int", T_INTEGER, 1, },
	{ PG_INT8OID, N(PG_INT8OID), "bigint", T_INTEGER, 0 },
	{ PG_NUMERICOID, N(PG_NUMERICOID), "numeric", T_DOUBLE, 0 },
	{ PG_FLOAT4OID, N(PG_FLOAT4OID), "real", T_DOUBLE, 1 }, /* or float4 */
	{ PG_FLOAT8OID, N(PG_FLOAT8OID), "double precision", T_DOUBLE, 0 }, /* or float8 */
	{ PG_TEXTOID, N(PG_TEXTOID), "text", T_STRING, 1 },
	{ PG_BPCHAROID, N(PG_BPCHAROID), "varchar", T_STRING, 0 },
	{ PG_VARCHAROID, N(PG_VARCHAROID), "varchar", T_CHAR, 1 },
	{ PG_BYTEAOID, N(PG_BYTEAOID), "bytea", T_BINARY, 1 },
	{ PG_BYTEAOID, N(PG_BYTEAOID), "blob", T_BINARY, 0 },
	{ TYPEMAP_TERM },
#if 0
	[PG_DATEOID] = "PG_DATEOID",
	[PG_TIMEOID] = "PG_TIMEOID",
	[PG_TIMESTAMPOID] = "PG_TIMESTAMPOID",
	[PG_TIMESTAMPZOID] = "PG_TIMESTAMPZOID",
	[PG_TIMETZOID] = "PG_TIMETZOID",
	[PG_OIDOID] = "PG_OIDOID",
	[PG_BITOID] = "PG_BITOID",
	[PG_VARBITOID] = "PG_VARBITOID",
#endif
};


 #ifdef BMYSQL_H
	// TODO: There MAY be a better way to lay out the mappings...
	static const int pgsql_mysql_auto_map[] = {
		[0] = 0,
		[PG_BOOLOID] = MYSQL_TYPE_TINY,
		[PG_BYTEAOID] = MYSQL_TYPE_BLOB,
		[PG_TEXTOID] = MYSQL_TYPE_STRING,
		[PG_BPCHAROID] = MYSQL_TYPE_VARCHAR,
		[PG_VARCHAROID] = MYSQL_TYPE_VARCHAR,
		[PG_INT2OID] = MYSQL_TYPE_SHORT,
		[PG_INT4OID] = MYSQL_TYPE_LONG,
		[PG_INT8OID] = MYSQL_TYPE_LONGLONG,
	#if 0
		[PG_OIDOID] = MYSQL_TYPE_LONG, // This is the unique column?
	#endif
		[PG_NUMERICOID] = MYSQL_TYPE_INT24,
		[PG_FLOAT4OID] = MYSQL_TYPE_FLOAT,
		[PG_FLOAT8OID] = MYSQL_TYPE_DOUBLE,
	#if 0
		[PG_DATEOID] = MYSQL_TYPE_DATE,
		[PG_TIMEOID] = MYSQL_TYPE_TIME,
		[PG_TIMESTAMPOID] = MYSQL_TYPE_TIMESTAMP,
		[PG_TIMESTAMPZOID] = MYSQL_TYPE_TIMESTAMP,
		[PG_TIMETZOID] = MYSQL_TYPE_TIMESTAMP,
	#endif
	#if 0
		[PG_BITOID] = MYSQL_TYPE_,
		[PG_VARBITOID] = MYSQL_TYPE_,
	#endif
	};
 #endif
#endif



#ifdef BMYSQL_H

#if 0
#define CHECK_MYSQL_TYPE( type, type_array ) \
	memchr( (unsigned char *)type_array, type, sizeof( type_array ) )

static const char * _mysql_string_map[] = {
	[0] = NULL,
	[MYSQL_TYPE_VAR_STRING] = "MYSQL_TYPE_VAR_STRING",
	[MYSQL_TYPE_STRING] = "MYSQL_TYPE_STRING",
	[MYSQL_TYPE_VARCHAR] = "MYSQL_TYPE_VARCHAR",
	[MYSQL_TYPE_FLOAT ] = "MYSQL_TYPE_FLOAT ",
	[MYSQL_TYPE_DOUBLE] = "MYSQL_TYPE_DOUBLE",
	[MYSQL_TYPE_DECIMAL ] = "MYSQL_TYPE_DECIMAL ",
	[MYSQL_TYPE_TINY] = "MYSQL_TYPE_TINY",
	[MYSQL_TYPE_SHORT  ] = "MYSQL_TYPE_SHORT  ",
	[MYSQL_TYPE_LONG] = "MYSQL_TYPE_LONG",
	[MYSQL_TYPE_NULL   ] = "MYSQL_TYPE_NULL   ",
	[MYSQL_TYPE_TIMESTAMP] = "MYSQL_TYPE_TIMESTAMP",
	[MYSQL_TYPE_LONGLONG] = "MYSQL_TYPE_LONGLONG",
	[MYSQL_TYPE_INT24] = "MYSQL_TYPE_INT24",
	[MYSQL_TYPE_NEWDECIMAL] = "MYSQL_TYPE_NEWDECIMAL",
	[MYSQL_TYPE_DATE   ] = "MYSQL_TYPE_DATE   ",
	[MYSQL_TYPE_TIME] = "MYSQL_TYPE_TIME",
	[MYSQL_TYPE_DATETIME ] = "MYSQL_TYPE_DATETIME ",
	[MYSQL_TYPE_YEAR] = "MYSQL_TYPE_YEAR",
	[MYSQL_TYPE_NEWDATE ] = "MYSQL_TYPE_NEWDATE ",
	[MYSQL_TYPE_VARCHAR] = "MYSQL_TYPE_VARCHAR",
	[MYSQL_TYPE_TINY_BLOB] = "MYSQL_TYPE_TINY_BLOB",
	[MYSQL_TYPE_MEDIUM_BLOB] = "MYSQL_TYPE_MEDIUM_BLOB",
	[MYSQL_TYPE_LONG_BLOB] = "MYSQL_TYPE_LONG_BLOB",
	[MYSQL_TYPE_BLOB] = "MYSQL_TYPE_BLOB",
};


/* yamcha */
static const unsigned char _mysql_strings[] = {
	MYSQL_TYPE_VAR_STRING,
	MYSQL_TYPE_STRING,
};

static const unsigned char _mysql_chars[] = {
	MYSQL_TYPE_VARCHAR,
};

static const unsigned char _mysql_doubles[] = {
	MYSQL_TYPE_FLOAT, 
	MYSQL_TYPE_DOUBLE
};

static const unsigned char _mysql_integers[] = {
	MYSQL_TYPE_DECIMAL, 
	MYSQL_TYPE_TINY,
	MYSQL_TYPE_SHORT,  
	MYSQL_TYPE_LONG,
	MYSQL_TYPE_NULL,   
	MYSQL_TYPE_TIMESTAMP,
	MYSQL_TYPE_LONGLONG,
	MYSQL_TYPE_INT24,
	MYSQL_TYPE_NEWDECIMAL
};

static const unsigned char _mysql_dates[] = {
	MYSQL_TYPE_DATE,   
	MYSQL_TYPE_TIME,
	MYSQL_TYPE_DATETIME, 
	MYSQL_TYPE_YEAR,
	MYSQL_TYPE_NEWDATE, 
	MYSQL_TYPE_VARCHAR,
};

static const unsigned char _mysql_blobs[] = {
	MYSQL_TYPE_TINY_BLOB,
	MYSQL_TYPE_MEDIUM_BLOB,
	MYSQL_TYPE_LONG_BLOB,
	MYSQL_TYPE_BLOB,
};

#endif

static const typemap_t mysql_map[] = {
	{ MYSQL_TYPE_TINY, N(MYSQL_TYPE_TINY), "TINYINT", T_INTEGER },
	{ MYSQL_TYPE_SHORT, N(MYSQL_TYPE_SHORT), "SMALLINT", T_INTEGER },
	{ MYSQL_TYPE_LONG, N(MYSQL_TYPE_LONG), "INT", T_INTEGER, 1 },
	{ MYSQL_TYPE_INT24, N(MYSQL_TYPE_INT24), "MEDIUMINT", T_INTEGER },
	{ MYSQL_TYPE_LONGLONG, N(MYSQL_TYPE_LONGLONG), "BIGINT", T_INTEGER },
	{ MYSQL_TYPE_DECIMAL, N(MYSQL_TYPE_DECIMAL), "DECIMAL", T_DOUBLE, 1 }, /* or NUMERIC */
	{ MYSQL_TYPE_NEWDECIMAL, N(MYSQL_TYPE_NEWDECIMAL), "NUMERIC", T_DOUBLE }, /* or NUMERIC */
	{ MYSQL_TYPE_FLOAT, N(MYSQL_TYPE_FLOAT), "FLOAT", T_DOUBLE },
	{ MYSQL_TYPE_DOUBLE, N(MYSQL_TYPE_DOUBLE), "DOUBLE", T_DOUBLE },
	{ MYSQL_TYPE_STRING, N(MYSQL_TYPE_STRING), "CHAR", T_CHAR, 1 },
	{ MYSQL_TYPE_VAR_STRING, N(MYSQL_TYPE_VAR_STRING), "VARCHAR", T_STRING, 1 },
	{ MYSQL_TYPE_BLOB, N(MYSQL_TYPE_BLOB), "BLOB", T_BINARY, 1 },
	{ TYPEMAP_TERM },
#if 0
	{ MYSQL_TYPE_TIMESTAMP, N(MYSQL_TYPE_TIMESTAMP), "TIMESTAMP", T_XXX },
	{ MYSQL_TYPE_DATE, N(MYSQL_TYPE_DATE), "DATE", T_XXX },
	{ MYSQL_TYPE_TIME, N(MYSQL_TYPE_TIME), "TIME", T_XXX },
	{ MYSQL_TYPE_DATETIME, N(MYSQL_TYPE_DATETIME), "DATETIME", T_XXX },
	{ MYSQL_TYPE_YEAR, N(MYSQL_TYPE_YEAR), "YEAR", T_XXX },
#endif
};

#if 0
static const char * _mysql_named_types[] = {
	[0] = NULL,
	[MYSQL_TYPE_TINY] = "TINYINT",
	[MYSQL_TYPE_SHORT] = "SMALLINT",
	[MYSQL_TYPE_LONG] = "INT",
	[MYSQL_TYPE_INT24] = "MEDIUMINT",
	[MYSQL_TYPE_LONGLONG] = "BIGINT",
	[MYSQL_TYPE_DECIMAL] = "DECIMAL", /* or NUMERIC */
	[MYSQL_TYPE_NEWDECIMAL] = "DECIMAL", /* or NUMERIC */
	[MYSQL_TYPE_FLOAT] = "FLOAT",
	[MYSQL_TYPE_DOUBLE] = "DOUBLE",
	[MYSQL_TYPE_STRING] = "CHAR",
	[MYSQL_TYPE_VAR_STRING] = "VARCHAR",
	/* Where is text? */	
	[MYSQL_TYPE_BLOB] = "BLOB",
	[MYSQL_TYPE_TIMESTAMP] = "TIMESTAMP",
	[MYSQL_TYPE_DATE] = "DATE",
	[MYSQL_TYPE_TIME] = "TIME",
	[MYSQL_TYPE_DATETIME] = "DATETIME",
	[MYSQL_TYPE_YEAR] = "YEAR",
#endif
#if 0
/* I can't reliably support these yet  */
	[MYSQL_TYPE_BIT] = "BIT",
MYSQL_TYPE_SET	SET
MYSQL_TYPE_ENUM	ENUM
MYSQL_TYPE_GEOMETRY	Spatial
MYSQL_TYPE_NULL	NULL-type
};
#endif

 #ifdef BPGSQL_H
	static const int _mysql_pgsql_auto_map[] = {
		//[0] = 0,
		[MYSQL_TYPE_VAR_STRING] = PG_TEXTOID,
		[MYSQL_TYPE_STRING] = PG_TEXTOID,
		[MYSQL_TYPE_VARCHAR] = PG_VARCHAROID,
		[MYSQL_TYPE_FLOAT] = PG_FLOAT4OID,
		[MYSQL_TYPE_DOUBLE] = PG_FLOAT8OID,
		[MYSQL_TYPE_DECIMAL] = PG_INT8OID,
		[MYSQL_TYPE_TINY] = PG_INT2OID,
		[MYSQL_TYPE_SHORT  ] = PG_INT4OID,
		[MYSQL_TYPE_LONG] = PG_INT8OID,
		//[MYSQL_TYPE_LONGLONG] = PG_xOID,
		[MYSQL_TYPE_TIMESTAMP] = PG_TIMESTAMPOID,
		[MYSQL_TYPE_INT24] = PG_INT8OID,
		[MYSQL_TYPE_NEWDECIMAL] = PG_NUMERICOID,
		[MYSQL_TYPE_DATE   ] = PG_DATEOID,
		[MYSQL_TYPE_TIME] = PG_TIMEOID,
		[MYSQL_TYPE_DATETIME ] = PG_TIMESTAMPOID,
		[MYSQL_TYPE_YEAR] = PG_DATEOID, // Might just be numeric
		[MYSQL_TYPE_NEWDATE ] = PG_DATEOID,
		[MYSQL_TYPE_TINY_BLOB] = PG_BYTEAOID,
		[MYSQL_TYPE_MEDIUM_BLOB] = PG_BYTEAOID,
		[MYSQL_TYPE_LONG_BLOB] = PG_BYTEAOID,
		[MYSQL_TYPE_BLOB] = PG_BYTEAOID,
	#if 0
		/* I can't reliably support these right now */
		[MYSQL_TYPE_SET] = PG_BYTEAOID,
		[MYSQL_TYPE_ENUM] = PG_BYTEAOID,
		[MYSQL_TYPE_GEOMETRY] = PG_BYTEAOID,
		[MYSQL_TYPE_NULL] = PG_BYTEAOID,
	#endif
	};
 #endif
#endif	


// Forward declarations for different output formats
void p_default( int, column_t * );
void p_xml( int, column_t * );
void p_comma( int, column_t * );
void p_cstruct( int, column_t * );
void p_carray( int, column_t * );
void p_sql( int, column_t * );
void p_json( int, column_t * );




sqldef_t coldefs[] = {
	/* sqlite */
	{ "%s INTEGER PRIMARY KEY AUTOINCREMENT\n", { "NULL",	"TEXT",	"TEXT",	"INTEGER","REAL","INTEGER", NULL, } },
	/* postgres */
	{ "%s SERIAL PRIMARY KEY\n", { "NULL",	"VARCHAR(1)",	"TEXT",	"INTEGER","DOUBLE","BOOLEAN", NULL, } },
	/* oracle */
	{ "%s INTEGER PRIMARY KEY AUTOINCREMENT", { "NULL",	"TEXT",	"TEXT",	"INTEGER","REAL","INTEGER", NULL, } },
	/* mssql */
	{ "%s INTEGER IDENTITY(1,1) NOT NULL\n", { "NULL",	"VARCHAR(1)",	"VARCHAR(MAX)",	"INTEGER","DOUBLE","BOOLEAN", NULL, } },
	/* mysql */
	{ "%s INTEGER NOT NULL\n", { "NULL",	"VARCHAR(1)",	"VARCHAR(MAX)",	"INTEGER","DOUBLE","BOOLEAN", NULL, } },
	/* sqlite */
};


// Global variables to ease testing
char delset[ 8 ] = { 13 /*'\r'*/, 10 /*'\n'*/, 0, 0, 0, 0, 0, 0 };
const char *STDIN = "/dev/stdin";
const char *STDOUT = "/dev/stdout";
char *no_header = "nothing";
char *output_file = NULL;
char *FFILE = NULL;
char *OUTPUT = NULL;
char *INPUT = NULL;
//char *DELIM = ",";
char *DELIM = ",";
char *prefix = NULL;
char *suffix = NULL;
char *datasource = NULL;
char *table = NULL;
char *stream_chars = NULL;
char *root = "root";
char *streamtype = NULL;
char *ld = "'", *rd = "'", *od = "'";
struct rep { char o, r; } ; //Ghetto replacement scheme...
struct rep **reps = NULL;
int create_new_dsn = 0;
int limit = 0;
int dump_parsed_dsn = 0;
int headers_only = 0;
int convert = 0;
int ascii = 0;
int newline = 1;
int typesafe = 0;
int schema = 0;
int serialize = 0;
int hlen = 0;
int no_unsigned = 0;
int adv = 0;
int show_stats = 0;
int stream_fmt = STREAM_PRINTF;
const char ucases[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
const char lcases[] = "abcdefghijklmnopqrstuvwxyz_";
const char exchrs[] = "~`!@#$%^&*()-_+={}[]|:;\"'<>,.?/";
type_t **gtypes = NULL;
int want_id = 0;
char *id_name = "id";
int want_datestamps = 0;
char *datestamp_name = "date";
int structdata=0;
int classdata=0;
case_t case_type = CASE_SNAKE;
sqltype_t dbengine = SQL_SQLITE3;
char *coercion = NULL;
char sqlite_primary_id[] = "id INTEGER PRIMARY KEY AUTOINCREMENT"; 


//Different languages need their own definitions
char * struct_coldefs[] = {
	"NULL",
	"char",
	"char *",
	"int",
	"double",
	"int",
	NULL,		
};


char * class_coldefs[] = {
	"NULL",
	"int",
	"String",
	"int",
	"double",
	"bool",
	NULL,		
};


char * sqlite_coldefs[] = {
	"NULL",
	"TEXT",
	"TEXT",
	"INTEGER",
	"REAL",
	"INTEGER",
	NULL,		
};


char * postgres_coldefs[] = {
	"NULL",
	"VARCHAR(1)",
	"VARCHAR(MAX)",
	"INTEGER",
	"DOUBLE",
	"BOOLEAN",
	NULL,		
};


char * mysql_coldefs[] = {
	"NULL",
	"TEXT",
	"TEXT",
	"INTEGER",
	"INTEGER",
	"INTEGER",
	NULL,		
};


char * oracle_coldefs[] = {
	"NULL",
	"TEXT",
	"TEXT",
	"INTEGER",
	"INTEGER",
	"INTEGER",
	NULL,		
};



// Convert word to snake_case
static void snakecase ( char **k ) {
	char *nk = *k;
	while ( *nk ) {
		char *u = (char *)ucases, *l = (char *)lcases;
		while ( *u ) {
			if ( *nk == *u ) {
				*nk = *l;
				break;
			}
			u++, l++;
		}
		nk++;
	}
}



// Convert word to camelCase
static void camelcase ( char **k ) {
	char *nk = *k;
	char *ok = *k;
	unsigned int plen = 0;

	while ( *ok ) {
		char *u = (char *)ucases; 
		char *l = (char *)lcases;

		// Replace any spaces or underscores
		if ( *ok == ' ' || *ok == '_' ) {
			ok++, *nk = *ok;
		#if 1
			// 
			while ( *l ) {
				if ( *nk == *l ) {
					*nk = *u;	
					break;
				}
				u++, l++;
			}
		#endif
			nk++, ok++, plen++;
		}
		else {
			// Find the lower case equivalent
			while ( *u ) {
				if ( *ok == *u ) { 
					*ok = *l;
					break;
				}
				u++, l++;
			}
		}

		*nk = *ok, nk++, ok++, plen++;
	}

#if 1
	if ( plen > 0 ) {
		(*k)[plen] = 0;
	}
#endif
}



// Duplicate a value
static char *dupval ( char *val, char **setval ) {
	if ( !val ) 
		return NULL;
	else {
		*setval = strdup( val );
		if ( !( *setval ) || !strlen( *setval ) )
			return NULL;	
		else if ( **setval == '-' ) { 
			return NULL;	
		}	
	}
	return *setval;
}



// Trim and copy a string, optionally using snake_case
static char * copy( char *src, int size, case_t t ) {
	char *b = NULL; 
	int nsize = 0;
	char *a = NULL; 
	if ( !( a = malloc( size + 1 ) ) || !memset( a, 0, size + 1 ) ) {
		return NULL;
	}
	b = (char *)trim( (unsigned char *)src, " ", size, &nsize );
	memcpy( a, b, nsize );
	if ( t == CASE_SNAKE ) {	
		snakecase( &a );
	}
	else if ( t == CASE_CAMEL ) {	
		camelcase( &a );
	}
	return a;
}




// Check the type of a value in a field
static type_t get_type ( char *v, type_t deftype ) {
	type_t t = T_INTEGER;

	// If the value is blank, then we need some help...
	// Use deftype in this case...
	if ( strlen( v ) == 0 ) {
//fprintf( stderr, "Value is blank or empty.\n" );
		return deftype;	
	}

	// If it's just one character, should check if it's a number or char
	if ( strlen( v ) == 1 ) {
		if ( !memchr( "0123456789", *v, 10 ) ) {
			t = T_CHAR;
		}
		return t;
	}

	// Check for booleans	
	if ( ( *v == 't' && !strcmp( "true", v ) ) || ( *v == 't' && !strcmp( "false", v ) ) ) {
		t = T_BOOLEAN;
		return t;
	}


	// Check for other stuff 
	for ( char *vv = v; *vv; vv++ ) {
		if ( *vv == '.' && ( t == T_INTEGER ) ) {
			t = T_DOUBLE;
		}
		if ( !memchr( "0123456789.-", *vv, 12 ) ) {
			t = T_STRING;
			return t;
		}
	}

	return t;
} 



// Simple key-value
void p_default( int ind, column_t *r ) {
	fprintf( stdout, "%s => %s%s%s\n", r->k, ld, r->v, rd );
}  



// JSON converter
void p_json( int ind, column_t *r ) {
	//Check if v is a number, null or some other value that should NOT be escaped
	fprintf( stdout, 
		"%s%c\"%s\": \"%s\"\n", &TAB[9-ind], adv ? ' ' : ',', r->k, r->v );
}  



// C struct converter
void p_carray ( int ind, column_t *r ) {
	fprintf( stdout, &", \"%s\""[ adv ], r->v );
}



// XML converter
void p_xml( int ind, column_t *r ) {
	fprintf( stdout, "%s<%s>%s</%s>\n", &TAB[9-ind], r->k, r->v, r->k );
}



// SQL converter
void p_sql( int ind, column_t *r ) {
	//Check if v is a number, null or some other value that should NOT be escaped
	if ( !typesafe ) {
		fprintf( stdout, &",%s%s%s"[adv], ld, r->v, rd );
	}
	else {
#if 1
		// Should check that they match too
		if ( r->type != T_STRING && r->type != T_CHAR )
			fprintf( stdout, &",%s"[adv], r->v );
		else {
			fprintf( stdout, &",%s%s%s"[adv], ld, r->v, rd );
		}
#endif
	}
}  



// Simple comma converter
void p_comma ( int ind, column_t *r ) {
	//Check if v is a number, null or some other value that should NOT be escaped
	fprintf( stdout, &",\"%s\""[adv], r->v );
}



// C struct converter
void p_cstruct ( int ind, column_t *r ) {
	//check that it's a number
	char *vv = (char *)r->v;

	//Handle blank values
	if ( !strlen( vv ) ) {
		fprintf( stdout, "%s", &TAB[9-ind] );
		fprintf( stdout, &", .%s = \"\"\n"[ adv ], r->k );
		return;
	}

#if 1
	fprintf( stdout, "%s", &TAB[9-ind] );
	fprintf( stdout, &", .%s = \"%s\"\n"[adv], r->k, r->v );
	return;
#else
	if ( !strcmp( vv, "true" ) || !strcmp( vv, "TRUE" ) ) {
		fprintf( stdout, "%s.%s = 1,\n", &TAB[9-ind], k );
		return;
	}
		
	if ( !strcmp( vv, "false" ) || !strcmp( vv, "FALSE" ) ) {
		fprintf( stdout, "%s.%s = 0,\n", &TAB[9-ind], k );
		return;
	}

	//Don't worry about typesafety
	if ( !typesafe ) {
		fprintf( stdout, "%s", &TAB[9-ind] );
		fprintf( stdout, &", .%s = \"%s\"\n"[adv], k, v );
		return;
	}
	
	//Handle typesafety
	while ( *vv ) {
		if ( !memchr( "0123456789", *vv, 10 ) ) {
			fprintf( stdout, "%s", &TAB[9-ind] );
			fprintf( stdout, &", .%s = \"%s\"\n"[adv], k, v );
			return;	
		}
		vv++;
	}
#endif
}




// Extract the headers from a CSV
char ** generate_headers( char *buf, char *del ) {
	char **headers = NULL;	
	zw_t p = { 0 };

	for ( int hlen = 0; strwalk( &p, buf, del ); ) {
		char *t = ( !p.size ) ? no_header : copy( &buf[ p.pos ], p.size - 1, case_type );
		add_item( &headers, t, char *, &hlen );
		if ( p.chr == '\n' || p.chr == '\r' ) break;
	}

	return headers;
}



// Extract the headers from a CSV
header_t ** generate_headers_from_text( char *buf, char *del ) {
	header_t **hxlist = NULL;	
	zw_t p = { 0 };

	for ( int hlen = 0; strwalk( &p, buf, del ); ) {
		char *t = ( !p.size ) ? no_header : copy( &buf[ p.pos ], p.size - 1, case_type );
		header_t *hx = malloc( sizeof( header_t ) );
		memset( hx, 0, sizeof( header_t ) );
		snprintf( hx->label, sizeof( hx->label ), "%s", t );
		add_item( &hxlist, hx, header_t *, &hlen );
		if ( p.chr == '\n' || p.chr == '\r' ) break;
	}

	return hxlist;
}



// Free headers allocated previously
void free_headers ( char **headers ) {
	char **h = headers;
	while ( *h ) {
		free( *h );
		h++;
	}
	free( headers );
}



// Get a value from a column
void extract_value_from_column ( char *src, char **dest, int size ) {
	if ( !reps && !no_unsigned ) 
		memcpy( *dest, src, size );
	else {
		char *pp = src;
		unsigned int k = 0;
		//just do two seperate passes... 
		//TODO: wow, this is bad...
		if ( no_unsigned ) {
			for ( int i = 0; i < size; i++ ) {
				if ( pp[ i ] < 32 || pp[ i ] > 126 )
					;// v->v[ i ] = ' ';
				else {
					*dest[ k ] = pp[ i ];
					k++;
				}
			}
		}

		if ( reps ) {
			if ( !k )
				k = size;
			else {
				pp = *dest; //v->v;
			}

			for ( unsigned int i = 0, l = 0; i<k; i++ ) {
				int j = 0;
				struct rep **r = reps;
				while ( (*r)->o ) {
					if ( pp[ i ] == (*r)->o ) {
						j = 1;		
						if ( (*r)->r != 0 ) {
							*dest[ l ] = (*r)->r;
							l++;
						}
						break;
					}
					r++;
				}
				if ( !j ) {
					*dest[ l ] = pp[ i ];
					l++;
				}
			}
		}
	}
}



//check for a valid stream
int check_for_valid_stream ( char *st ) {
	for ( streamtype_t *s = streams; s->type; s++ ) {
		if ( !strcmp( s->type, st ) ) {
			return s->streamfmt;
		} 
	} 
	return -1;
}




// Extract one row only
static char * extract_row( char *buf, int buflen ) {
	// Extract just the first row of real values
	char *start = memchr( buf, '\n', buflen );  
	int slen = 0;
	for ( char *s = ++start; ( *s != '\n' && *s != '\r' ); s++, slen++ );

	// 
	slen++;
	char *str = malloc( slen + 1 );
	memset( str, 0, slen + 1 );
	memcpy( str, start, slen );
	return str;
}



// Return all the types
static type_t ** get_types ( char *str, const char *delim ) {
	//... 
	type_t **ts = NULL;
	int tlen = 0;
	zw_t p = { 0 };

	while ( strwalk( &p, str, delim ) ) {
		char *t = NULL;
		int len = 0;

		if ( !p.size ) {
			type_t *p = malloc( sizeof( type_t ) );
			memset( p, 0, sizeof( type_t ) );
			*p = T_STRING;
			add_item( &ts, p, type_t *, &tlen );
			//fprintf( stderr, "%s -> %s ( %d )\n", *j, "", (int)*p); j++;
			continue;
		}

		t = copy( &str[ p.pos ], p.size - 1, 0 );
		//fprintf( stderr, "%d\n", ( len = strlen( t ) ) );
		if ( ( len = strlen( t ) ) == 0 ) {
			// ?
			type_t *p = malloc( sizeof( type_t ) );
			memset( p, 0, sizeof( type_t ) );
			*p = T_STRING;
			add_item( &ts, p, type_t *, &tlen );
			free( t );
			//fprintf( stderr, "%s -> %s ( %d )\n", *j, "", (int)*p); j++;
			continue;
		}

		type_t type = get_type( t, T_STRING ); 
		type_t *x = malloc( sizeof( type_t ) );
		memset( x, 0, sizeof( type_t ) );
		*x = type;

		//fprintf( stderr, "%s -> %s ( %d )\n", *j, t, (int)type ); j++;
		add_item( &ts, x, type_t *, &tlen );
		free( t );
	}

	return ts;
}



#if 0
int ascii_f ( const char *file, const char *delim, const char *output ) {
	unsigned char *buf = NULL;
	char err[ ERRLEN ] = { 0 };
	int buflen = 0, len = 0;
	#if 0
	int fd = -1; 
	#endif

	// Check for bad characters in the delimiter
	for ( const char *d = delim; *d; d++ ) {
		if ( *d > 127 ) {
			return nerr( "delimiter contains something not ASCII\n" );
		}
	}

	// Load the entire file
	if ( !( buf = read_file( file, &buflen, err, sizeof( err ) ) ) ) {
		return nerr( "%s\n", err );
	}

	#if 0
	// Open whatever you asked for 
	if ( ( fd = open( output, O_RDWR | O_CREAT, 0755 ) ) == -1 ) {
		return nerr( "Failed to open file %s: %s\n", output, strerror( errno ) );
	}
	#endif

	// Check for bad characters in the file
	// NOTE: Written this way b/c *d can easily be \0 if we're running this...
	for ( const unsigned char *d = buf; buflen; buflen--, len++, d++ ) {
	#if 0
		// Maybe you need to stop here?  Or convert certain byte sequences? 
	#endif
		if ( *d < 128 && ( write( 1, d, 1 ) == -1 ) ) {
			return nerr( "Failed to write byte %d from %s to new file %s: %s\n", 
				len, file, output, strerror( errno ) );
		}
	}

	#if 0
		// Using another file, you can close it here 
	#endif

	free( buf );
	return 1;
}
#endif


// A function to safely copy a string with an optional delimiter
int safecpypos( const char *src, char *dest, char *delim, int maxlen ) {

	unsigned int len = maxlen;
	unsigned int dpos = 0;
	unsigned int copied = 0;

	// Stop if neither of these are specified
	if ( !src || !dest ) {
		return 0;
	}

	// Find delim, if not use maxlen
	if ( delim && memchr( src, *delim, strlen( src ) ) ) {
		// TODO: Can't we subtract pointers to get this count?
		// Get the position
		const char *s = src;
		while ( *s ) {
			if ( *s == *delim ) break; 
			s++, dpos++;
		}

		// Break if dpos > maxlen
		if ( dpos > maxlen ) {
			return 0;
		}

		len = dpos;
	}

	// Finally, copy the string into whatever destination
	// TODO: This can fail, so consider that
	memcpy( dest, src, len );
	copied = strlen( dest );	
	return copied;
}


int safecpynumeric( char *numtext ) {
	for ( char *p = numtext; *p; p++ ) {
		if ( !memchr( "0123456789", *p, 10 ) ) return -1;
	}

	return atoi( numtext );
}


/*
 * Parse DSN info the above structure
 *
 * We only support URI styles for consistency.
 * 
 * NOTE: Postgres seems to include support for connecting to multiple hosts
 * This doesn't make sense to support here...
 * 
 */
int parse_dsn_info( dsn_t *conn, char *err, int errlen ) {
	
	// Define	
	//dbtype_t type = DB_NONE;
	int lp = -1;
	char port[ 6 ] = {0};
	char *dsn = NULL;
	unsigned int dsnlen = 0;


	// Cut out silly stuff
	if ( !( dsn = conn->connstr ) || ( dsnlen = strlen( dsn ) ) < 1 ) {
		snprintf( err, errlen, "Source string is either unspecified or zero-length..." );
		return 0;
	}

	
	// Get the database type
	if ( 0 ) 0;
#ifdef BMYSQL_H
	//if ( strlen( dsn ) > 8 && !memcmp( dsn, "mysql://", 8 ) ) 
	if ( STRLCMP( dsn, "mysql://" ) )
		conn->type = DB_MYSQL, conn->port = 3306, dsn += 8, dsnlen -= 8;
#endif
#ifdef BPGSQL_H
	else if ( STRLCMP( dsn, "postgresql://" ) )
	//else if ( strlen( dsn ) > 13 && !memcmp( dsn, "postgresql://", strlen( "postgresql://" ) ) ) 
		conn->type = DB_POSTGRESQL, conn->port = 5432, dsn += 13, dsnlen -= 13;
	else if ( STRLCMP( dsn, "postgres://" ) )
	//else if ( strlen( dsn ) > 11 && !memcmp( dsn, "postgres://", strlen( "postgres://" ) ) ) 
		conn->type = DB_POSTGRESQL, conn->port = 5432, dsn += 11, dsnlen -= 11;
#endif
	else if ( STRLCMP( dsn, "sqlite3://" ) )
	//else if ( strlen( dsn ) > 10 && !memcmp( dsn, "sqlite3://", strlen( "sqlite3://" ) ) )
		conn->type = DB_SQLITE, conn->port = 0, dsn += 10, dsnlen -= 10;
	else {
		if ( strlen( dsn ) > 7 && !memcmp( dsn, "file://", 7 ) ) {
			conn->connstr += 7, dsnlen -= 7;
		}
		else if ( strlen( dsn ) == 1 && *dsn == '-' ) {
			conn->connstr = "/dev/stdin";
		}

		// TODO: Check valid extensions
		conn->type = DB_FILE; 
		conn->port = 0; 
		return 1;
	}

	// Maybe just die if it is blank.
	if ( *dsn == '\0' ) {
		return 1;
	}

	// Handle the different string types as best you can
	// A) localhost, localhost:port doesn't make much sense...
	if ( !memchr( dsn, ':', dsnlen ) && !memchr( dsn, '@', dsnlen ) && !memchr( dsn, '/', dsnlen ) ) {
		//lp = safecpypos( dsn, conn->hostname, NULL, sizeof( conn->hostname ) ); 	
		snprintf( conn->hostname, sizeof( conn->hostname ), "%s", dsn );
	} 

	// C) localhost/database, localhost:port/database
	else if ( memchr( dsn, '/', dsnlen ) && !memchr( dsn, '@', dsnlen ) ) {
		// Copy the port
		if ( !memchr( dsn, ':', dsnlen ) )  {
			lp = safecpypos( dsn, conn->hostname, "/", sizeof( conn->hostname ) );
			dsn += lp + 1;
			snprintf( conn->dbname, sizeof( conn->dbname ), "%s", dsn );
		}
		else {
			lp = safecpypos( dsn, conn->hostname, ":", sizeof( conn->hostname ) );
			dsn += lp + 1;

			// Handle port
			lp = safecpypos( dsn, port, "/", 256 );
			for ( char *p = port; *p; p++ ) {
				if ( !memchr( "0123456789", *p, 10 ) ) {
					snprintf( err, errlen, "Specified port number in DSN is invalid." );
					return 0;
				}
			}
			conn->port = atoi( port );

			// Move and copy dbname and options if any
			dsn += lp + 1;
			lp = safecpypos( dsn, conn->dbname, "?", sizeof( conn->dbname ) );
		}
	}
 
	// E) [ user:secret ] @localhost/db[ .table ]
	else if ( memchr( dsn, '@', dsnlen ) ) {

		int p = 0;	
		char a[ 2048 ], b[ 2048 ], *aa = NULL, *bb = NULL, *opts = NULL, *end = NULL;
		memset( a, 0, sizeof( a ) );
		memset( b, 0, sizeof( b ) );
		aa = a, bb = b;

		// Stop if we get something weird
		if ( memchrocc( dsn, '@', dsnlen ) > 1 ) {
			snprintf( err, errlen, "'@' character should only appear once" );
			return 0;
		}

		// Split the two halves of a proper DSN
		if ( ( p = memchrat( dsn, '@', dsnlen ) ) < sizeof( a ) ) {
			memcpy( a, dsn, p );
		}

		// Copy the second half after @ 
		if ( ( dsnlen - ( p + 1 ) ) < sizeof( b ) ) {
			memcpy( b, &dsn[ p + 1 ], dsnlen - ( p + 1 ) );
		}

		// Get user and optional password
		if ( memchr( a, ':', sizeof( a ) ) ) {
			p = safecpypos( aa, conn->username, ":", sizeof( conn->username ) ); 
			if ( !p ) {
				snprintf( err, errlen, 
					"Username in DSN is too large for buffer (max %ld chars).", 
					sizeof( conn->username ) 
				);
				return 0;
			}
			aa += p + 1;

			if ( !safecpypos( aa, conn->password, NULL, sizeof( conn->password ) ) ) {
				snprintf( err, errlen, 
					"Password in DSN is too large for buffer (max %ld chars).", 
					sizeof( conn->password ) 
				);
				return 0;
			}
		}
		else {
			if ( !safecpypos( a, conn->username, NULL, sizeof( conn->username ) ) ) {
				snprintf( err, errlen, 
					"Username in DSN is too large for buffer (max %ld chars)", 
					sizeof( conn->username ) 
				);
				return 0;
			}
		}

		// Moved this to the beginning to keep repetition to a minimum. 
		opts = memchr( bb, '?', strlen( bb ) );
		end = opts ? "?" : NULL;
	
		// If no database was specified, just process the hostname, port and options
		if ( !memchr( bb, '/', strlen( bb ) ) ) {
			if ( !memchr( bb, ':', strlen( bb ) ) ) {
				p = safecpypos( bb, conn->hostname, end, sizeof( conn->hostname ) ), bb += p + 1;
			}
			else {
				p = safecpypos( bb, conn->hostname, ":", sizeof( conn->hostname ) ), bb += p + 1;

				if ( !( p = safecpypos( bb, port, end, sizeof( port ) ) ) ) {
					snprintf( err, errlen, "Port number given in DSN is invalid." );
					return 0;
				}

				if ( ( conn->port = safecpynumeric( port ) ) == -1 || conn->port > 65535 ) {
					snprintf( err, errlen, "Port number given in DSN (%s) is invalid.", port );
					return 0;
				}
			}
		}
		else {
			// Get hostname and all of the other stuff if it's there
			// hostname, port, and / (and maybe something else)
			if ( memchr( bb, ':', strlen( bb ) ) ) {
				
				// copy hostname first
				p = safecpypos( bb, conn->hostname, ":", sizeof( conn->hostname ) ), bb += p + 1;

				// hostname should always exist
				if ( !( p = safecpypos( bb, port, "/", sizeof( port ) ) ) ) {
					snprintf( err, errlen, "Port number given in DSN is invalid." );
					return 0;
				}

				// If this is not all 
				bb += p + 1;
				if ( ( conn->port = safecpynumeric( port ) ) == -1 || conn->port > 65535 ) {
					snprintf( err, errlen, "Port number given in DSN (%s) is invalid.", port );
					return 0;
				}

				// Extract just the database name
				if ( !memchr( bb, '.', strlen( bb ) ) ) { 
					if ( !( p = safecpypos( bb, conn->dbname, NULL, sizeof( conn->dbname ) ) ) ) {
						snprintf( err, errlen, "Database name is too large for buffer." );
						return 0;
					}
				}
				else {
					if ( !( p = safecpypos( bb, conn->dbname, ".", sizeof( conn->dbname ) ) ) ) {
						snprintf( err, errlen, "Database name is too large for buffer." );
						return 0;
					}
					bb += p + 1;
					//if ( !( p = safecpypos( bb, conn->tablename, "?", sizeof( conn->tablename ) ) ) ) {
					if ( !( p = safecpypos( bb, conn->tablename, memchr( bb, '?', strlen( bb ) ) ? "?" : NULL, sizeof( conn->tablename ) ) ) ) {
						snprintf( err, errlen, "Table name is too large for buffer." );
						return 0;
					}
				}
			}

			//
			else if ( memchr( bb, '/', strlen( bb ) ) ) {
				if ( !( p = safecpypos( bb, conn->hostname, "/", sizeof( conn->hostname ) ) ) ) {
					snprintf( err, errlen, "Hostname too large for buffer (max %ld chars).", 
						sizeof( conn->hostname ) );
					return 0;
				}

				bb += p + 1;
			
				// Extract just the database name
				if ( !memchr( bb, '.', strlen( bb ) ) ) { 
					if ( !( p = safecpypos( bb, conn->dbname, NULL, sizeof( conn->dbname ) ) ) ) {
						snprintf( err, errlen, "Database name is too large for buffer." );
						return 0;
					}
				}
				else {
					if ( !( p = safecpypos( bb, conn->dbname, ".", sizeof( conn->dbname ) ) ) ) {
						snprintf( err, errlen, "Database name is too large for buffer." );
						return 0;
					}
					bb += p + 1;
					if ( !( p = safecpypos( bb, conn->tablename, memchr( bb, '?', strlen( bb ) ) ? "?" : NULL, sizeof( conn->tablename ) ) ) ) {
						snprintf( err, errlen, "Table name is too large for buffer." );
						return 0;
					}
				}
			} /*memchr / */

			if ( opts ) {
				snprintf( conn->connoptions, sizeof( conn->connoptions ), "%s", ++opts );
			}
	
		#ifdef BPGSQL_H
			// If it's a PG string, we can fix it here...
			if ( conn->type == DB_POSTGRESQL ) {
				// Fix the connection string if it hasn't been done yet...
				char *start = NULL, *tstart = NULL, *qstart = NULL;
				if ( ( start = memchr( conn->connstr, '@', strlen( conn->connstr ) ) ) ) {
			
				// Find this first	
				if ( ( start = memchr( start, '/', strlen( start ) ) ) ) {	
					// If the table has been specified, get rid of everything before the '?'
					if ( ( tstart = memchr( start, '.', strlen( start ) ) ) ) {
						// Terminate the string and disregard the rest of the string for initializing PG
						if ( !( qstart = memchr( start, '?', strlen( start ) ) ) ) {
							*tstart = '\0';
						}
						else {
							int qlen = strlen( qstart );
							memmove( tstart, qstart, qlen ); 
							*(tstart + qlen) = '\0';
						}

						//fprintf( stdout, "CONN = %s\n", conn->connstr );
					}
				}
				}
			}
		#endif
		}
	}
		
	return 1;
}



#ifndef DEBUG_H
 #define print_dsn(A) 0
 #define print_headers(A) 0 
 #define print_stream_type(A) 0
#else

// Dump the DSN
void print_dsn ( dsn_t *conninfo ) {
	fprintf( stdout, "dsn->connstr:    '%s'\n", conninfo->connstr );	
	fprintf( stdout, "dsn->type:       %s\n",   dbtypes[ conninfo->type ] );
	fprintf( stdout, "dsn->username:   '%s'\n", conninfo->username );
	fprintf( stdout, "dsn->password:   '%s'\n", conninfo->password );
	fprintf( stdout, "dsn->hostname:   '%s'\n", conninfo->hostname );
	fprintf( stdout, "dsn->dbname:     '%s'\n", conninfo->dbname );
	fprintf( stdout, "dsn->tablename:  '%s'\n", conninfo->tablename );
	//fprintf( stdout, "dsn->socketpath: '%s'\n", conninfo->socketpath );
	fprintf( stdout, "dsn->port:       %d\n", conninfo->port );
	fprintf( stdout, "dsn->conn:       %p\n", conninfo->conn );	
	fprintf( stdout, "dsn->options:    '%s'\n", conninfo->connoptions );	
}


// Dump any headers
void print_headers( dsn_t *t ) {
	for ( header_t **s = t->headers; s && *s; s++ ) {
		printf( "%s %d\n", (*s)->label, (*s)->type );
	}
}


void print_stream_type( stream_t t ) {
	if ( t == STREAM_NONE ) printf( "STREAM_NONE\n" );
	else if ( t == STREAM_PRINTF ) printf( "STREAM_PRINTF\n" );
	else if ( t == STREAM_JSON ) printf( "STREAM_JSON\n" );
	else if ( t == STREAM_XML ) printf( "STREAM_XML\n" );
	else if ( t == STREAM_SQL ) printf( "STREAM_SQL\n" );
	else if ( t == STREAM_CSTRUCT ) printf( "STREAM_CSTRUCT\n" );
	else if ( t == STREAM_COMMA ) printf( "STREAM_COMMA\n" );
	else if ( t == STREAM_CARRAY ) printf( "STREAM_CARRAY\n" );
	else if ( t == STREAM_JCLASS ) printf( "STREAM_JCLASS\n" );
#ifdef BPGSQL_H
	else if ( t == STREAM_PGSQL ) printf( "STREAM_PGSQL\n" );
#endif
#ifdef BMYSQL_H
	else if ( t == STREAM_MYSQL ) printf( "STREAM_MYSQL\n" );
#endif
	else if ( t == STREAM_CUSTOM ) {
		printf( "STREAM_CUSTOM\n" );
	}
}
#endif




void destroy_dsn_headers ( dsn_t *t ) {
	for ( header_t **s = t->headers; s && *s; s++ ) {
		free( *s );
	}
	free( t->headers );
}



// Destroy all records
void destroy_dsn_rows ( dsn_t *t ) {
	for ( row_t **r = t->rows; r && *r; r++ ) {
		for ( column_t **c = (*r)->columns; c && *c; c++ ) free( (*c) );	
		free( (*r)->columns );
		free( (*r) );
	}
	free( t->rows );
}


/**
 * schema_from_dsn( dsn_t * )
 * ===================
 *
 * Close a data source.
 * 
 */
int schema_from_dsn( dsn_t *iconn, dsn_t *oconn, char *fm, int fmtlen, char *err, int errlen ) {

	// Create the table for whatever the source may be 
	// fprintf( stdout, "DROP TABLE IF EXISTS %s;\n", conn->tablename );
	//fprintf( stdout, "CREATE TABLE IF NOT EXISTS %s (\n", conn->tablename );

	// Depends on output chosen
	int len = iconn->hlen;
	int written = 0;
	char *fmt = fm;
	//const typemap_t *cdefs = NULL;
	//const typemap_t *matchdefs = NULL;
	const char stmtfmt[] = "CREATE TABLE IF NOT EXISTS %s (\n"; 

	// Schema creation won't work if no table name is specified.  Stop here if that's so.
	if ( !strlen( oconn->tablename ) ) {
		snprintf( err, errlen, "No table name was specified." );	
		return 0;
	}

	// Write the first line 
	written = snprintf( fmt, fmtlen, stmtfmt, iconn->tablename );
	fmtlen -= written, fmt += written;

#if 0
	// Choose an engine according to type
	if ( oconn->type == DB_MYSQL )
		cdefs = mysql_map;
	else if ( oconn->type == DB_POSTGRESQL )
		cdefs = pgsql_map;
	else if ( oconn->type == DB_SQLITE )
		cdefs = mysql_map;
	else {
		snprintf( err, errlen, "Invalid SQL engine selected." );	
		return 0;
	}
#endif

	// Add an ID if we specify it? (unsure how to do this)
	if ( want_id ) {
		sqldef_t *col = &coldefs[ dbengine ];
		fprintf( stdout, col->id_fmt, id_name );
		adv = 0;
	}

	//
	if ( want_datestamps ) {
		( adv == 1 ) ? 0 : fprintf( stdout, "," );
		fprintf( stdout, "%s_created INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP\n", datestamp_name );
		fprintf( stdout, ",%s_updated INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP\n", datestamp_name );
		adv = 0;
	}


	// SQL schema creation will
	// Generate a line for each row
	for ( header_t **s = iconn->headers; s && *s; s++, len-- ) {
		// Getting the type needs to be a bit smarter now
		typemap_t *type = NULL;

		if ( (*s)->ctype )
			type = (*s)->ctype;
		else if ( !( type = get_typemap_by_btype( oconn->typemap, (*s)->type ) ) ) {
			const char fmt[] = "No available type found for column '%s'";
			snprintf( err, errlen, fmt, (*s)->label );
			return 0;
		}
	#if 0
		/* map i->db type to o->db type here */
	#endif

		//const char fmt[] = "\t%s %s";
		written = snprintf( fmt, fmtlen, "\t%s %s", (*s)->label, type->typename );
		fmtlen -= written, fmt += written;

		// Are we trying to populate or create a MySQL database
		// Does anyone else support this syntax?
		if ( len == 1 && (*s)->primary ) {
			written = snprintf( fmt, fmtlen, "%s", "PRIMARY KEY" );
			fmtlen -= written, fmt += written;
		}
	
		if ( len > 1 ) {
			written = snprintf( fmt, fmtlen, "%s", "," );
			fmtlen -= written, fmt += written;
		}

		written = snprintf( fmt, fmtlen, "\n" );
		fmtlen -= written, fmt += written;
	}

	// Terminate the schema
	written = snprintf( fmt, fmtlen, ");\n" );
	fmtlen -= written, fmt += written;
	return 1;
}



/**
 * create_dsn( dsn_t * )
 * ===================
 *
 * Create a data source.  (A file, database if SQLite3 or
 * a table if a super heavy weight database.)
 * 
 */
int create_dsn ( dsn_t *oconn, dsn_t *iconn, char *err, int errlen ) {
	// This might be a little harder, unless there is a way to either create a new connection
	char schema_fmt[ MAX_STMT_SIZE ];
	memset( schema_fmt, 0, MAX_STMT_SIZE );
#if 0
	const char create_database_stmt[] = "CREATE DATABASE IF NOT EXISTS %s";
#endif

	// File? (just open a new file?)
	if ( oconn->type == DB_FILE ) {

	}


	// Every other engine will need a schema
	if ( !schema_from_dsn( iconn, oconn, schema_fmt, sizeof( schema_fmt ), err, errlen ) ) {
		return 0;
	}


fprintf( stdout, "SCHEMA %s\n", schema_fmt );


	// SQLite: create table (db ought to be created when you open it)
	if ( oconn->type == DB_SQLITE ) {
		
	}


#ifdef BMYSQL_H
	// MySQL: create db & table (use an if not exists?, etc)
	else if ( oconn->type == DB_MYSQL ) {
		// Try to connect to instance first
		MYSQL *t = NULL;
		MYSQL *myconn = NULL;

		// Initialize a new connection
		if ( !( myconn = mysql_init( 0 ) ) ) {
			const char fmt[] = "Couldn't initialize MySQL connection: %s";
			snprintf( err, errlen, fmt, mysql_error( myconn ) );
			return 0;
		}

		// Easiest to just cancel if no database is specified.  
		if ( !strlen( oconn->dbname ) ) {
			snprintf( err, errlen, "Database name was not specified for output datasource" );
			return 0;
		}

		// Then try to create the db
		t = mysql_real_connect( 
			myconn, 
			oconn->hostname, 
			strlen( oconn->username ) ? oconn->username : NULL, 
			strlen( oconn->password ) ? oconn->password : NULL, 
			oconn->dbname,	
			oconn->port, 
			NULL, 
			0 
		);

		// Then try to create the table
		// Check the connection
		if ( !t ) {
			const char fmt[] = "Couldn't connect to server: %s";
			snprintf( err, errlen, fmt, mysql_error( myconn ) );
			mysql_close( myconn );
			return 0;
		}

		// Create the table in mysql
		if ( mysql_query( myconn, schema_fmt ) > 0 ) {
			const char fmt[] = "Failed to run query against selected db and table: %s"; 
			snprintf( err, errlen, fmt, mysql_error( myconn ) );
			mysql_close( myconn );
			return 0;
		}

		// Close the MS connection
		mysql_close( myconn );
	}
#endif


#ifdef BPGSQL_H
	// Postgres: create db & table (use an if not exists?, etc)
	else if ( oconn->type == DB_POSTGRESQL ) {
		// Try to connect to insetance first
		PGconn *pgconn = NULL;
		PGresult *pgres = NULL;
		char *cs = NULL;
		char cs_buffer[ MAX_STMT_SIZE ];
		memset( cs_buffer, 0, MAX_STMT_SIZE );

		// Handle cases in which the database does not exist
		if ( strlen( oconn->dbname ) ) {
			cs = oconn->connstr;
		}
		else {
			const char defname[] = "postgres";

			// This is going to prove difficult too.
			// Since Postgres forces us to choose a database
			if ( !strlen( oconn->username ) && !strlen( oconn->password ) )
				snprintf( cs_buffer, sizeof( cs_buffer ), "host=%s port=%d dbname=%s", oconn->hostname, oconn->port, defname );
			else if ( !strlen( oconn->password ) )
				snprintf( cs_buffer, sizeof( cs_buffer ), "host=%s port=%d user=%s dbname=%s", oconn->hostname, oconn->port, oconn->username, defname );
			else {
				const char fmt[] = "host=%s port=%d user=%s password=%s dbname=%s"; 
				snprintf( cs_buffer, sizeof( cs_buffer ), fmt, oconn->hostname, oconn->port, oconn->username, oconn->password, defname );
			}
			cs = cs_buffer;
		}
fprintf( stdout, "CS = %s\n", cs );
		
		//if ( PQstatus( ( pgconn = PQconnectdb( conn->connstr ) ) ) != CONNECTION_OK ) {
		if ( PQstatus( ( pgconn = PQconnectdb( cs ) ) ) != CONNECTION_OK ) {
			const char fmt[] = "Couldn't initialize PostsreSQL connection: %s";
			snprintf( err, errlen, fmt, PQerrorMessage( pgconn ) );
			return 0;
		}

//fprintf( stdout, "OUTPUT CONN => %p\n", pgconn );
//fprintf( stdout, "SCHEMA => %s\n", schema_fmt );
#if 1
		// Check if db exists (if not create it) 
		if ( !( pgres = PQexec( pgconn, schema_fmt ) ) ) {
			const char fmt[] = "Failed to run query against selected db and table: '%s'";
			snprintf( err, errlen, fmt, PQerrorMessage( pgconn ) );
			PQfinish( pgconn );
			return 0;
		}
#endif

		// Need to clear and preferably destroy it...
		PQclear( pgres );
		PQfinish( pgconn );

		// Set both of these
		//conn->conn = pgconn;
		//conn->res = pgres;
	}
#endif

	return 1;
}


//
int modify_pgconnstr ( char *connstr, int fmtlen ) {
	char *start = NULL, *tstart = NULL, *qstart = NULL;
	if ( ( start = memchr( connstr, '@', strlen( connstr ) ) ) ) {
	#if 1
		if ( ( tstart = memchr( start, '.', strlen( start ) ) ) ) {
			// Terminate the string and disregard the rest of the string for initializing PG
			if ( !( qstart = memchr( start, '?', strlen( start ) ) ) ) {
				*tstart = '\0';
			}
			else {
				int qlen = strlen( qstart );
				memmove( tstart, qstart, qlen ); 
				*(tstart + qlen) = '\0';
			}

			//fprintf( stdout, "CONN = %s\n", connstr );
		}
	#endif
	}
return 1;
}



/**
 * open_dsn( dsn_t * )
 * ===================
 *
 * Open and manage a data source.
 * 
 */
int open_dsn ( dsn_t *conn, const char *qopt, char *err, int errlen ) {
	char query[ 2048 ];
	memset( query, 0, sizeof( query ) );

	// Handle opening the database first
	if ( conn->type == DB_FILE ) {
		DPRINTF( "Opening file at %s\n", conn->connstr );

		// Do an access or stat here?
		if ( access( conn->connstr, F_OK | R_OK ) == -1 ) {
			// Optionally, check the format and die with the right message
			snprintf( err, errlen, "%s", strerror( errno ) );
			return 0;
		}
			
		// TODO: Allocating everything like this is fast, but not efficient on memory
		if ( !( conn->conn = read_file( conn->connstr, &conn->clen, err, errlen ) ) ) {
			//snprintf( stderr, "%s\n", err ); // This won't happen twice...
			return 0;
		}

#if 0
		int *fd = malloc( sizeof( int ) );

		if ( ( *fd = open( conn->connstr, O_RDONLY ) ) ) {
			snprintf( err, errlen, "%s\n", strerror( errno ) );
			return 0;
		}

		conn->conn = fd;
#endif

#if 0
		// Use fopen instead
		if ( !( conn->conn = fopen( conn->connstr, "r" ) ) ) {
			snprintf( err, errlen, "%s", strerror( errno ) );
			return 0;
		}
#endif
		return 1;
	}


	// Stop if no table name was specified
	if ( !table && ( !( *conn->tablename ) || strlen( conn->tablename ) < 1 ) ) {
		snprintf( err, errlen, "No table name specified for source data." );
		return 0;
	}

	// Create the final query from here too
	if ( qopt ) 
		snprintf( query, sizeof( query ) - 1, "%s", qopt );
	else {
		snprintf( query, sizeof( query ) - 1, "SELECT * FROM %s", conn->tablename );
	}

#ifdef BMYSQL_H
	if ( conn->type == DB_MYSQL ) {
		// Define
		MYSQL *myconn = NULL; 
		MYSQL *t = NULL; 
		MYSQL_RES *myres = NULL; 

		//
		conn->defs = mysql_coldefs;

		// Initialize the connection
		if ( !( conn->conn = myconn = mysql_init( 0 ) ) ) {
			const char fmt[] = "Couldn't initialize MySQL connection: %s";
			snprintf( err, errlen, fmt, mysql_error( myconn ) );
			mysql_close( myconn );
			return 0;
		}

		// Open the connection
		t = mysql_real_connect( 
			conn->conn, 
			conn->hostname, 
			conn->username, 
			conn->password, 
			conn->dbname, 
			conn->port, 
			NULL, 
			0 
		);

		// Check the connection
		if ( !t ) {
			const char fmt[] = "Couldn't connect to server: %s";
			snprintf( err, errlen, fmt, mysql_error( myconn ) );
			return 0;
		}

		// Execute whatever query
		if ( mysql_query( myconn, query ) > 0 ) {
			const char fmt[] = "Failed to run query against selected db and table: %s"; 
			snprintf( err, errlen, fmt, mysql_error( myconn ) );
			mysql_close( myconn );
			return 0;
		}

		// Use the result
		if ( !( myres = mysql_store_result( myconn ) ) ) {
			const char fmt[] = "Failed to store results set from last query: %s"; 
			snprintf( err, errlen, fmt, mysql_error( myconn ) );
			mysql_close( myconn );
			return 0;
		}

		// Set both of these
		conn->conn = myconn;
		conn->res = myres;
	}
#endif

#ifdef BPGSQL_H
	if ( conn->type == DB_POSTGRESQL ) {
		// Define	
		PGconn *pgconn = NULL;
		PGresult *pgres = NULL;
		char *connstr = conn->connstr;

		// ...
		conn->defs = postgres_coldefs;

	#if 0
		char *start = NULL, *tstart = NULL, *qstart = NULL;
		//if ( connstr && modify_pgconnstr( &connstr );
		// Fix the connection string if it hasn't been done yet...
		if ( connstr && ( start = memchr( connstr, '@', strlen( connstr ) ) ) ) {
			// If the table has been specified, get rid of everything before the '?'
			if ( ( tstart = memchr( start, '.', strlen( start ) ) ) ) {
				// Terminate the string and disregard the rest of the string for initializing PG
				if ( !( qstart = memchr( start, '?', strlen( start ) ) ) ) {
					*tstart = '\0';
				}
				else {
					int qlen = strlen( qstart );
					memmove( tstart, qstart, qlen ); 
					*(tstart + qlen) = '\0';
				}

				//fprintf( stdout, "CONN = %s\n", connstr );
			}
		}
	#endif

		// Attempt to open the connection and handle errors
		if ( PQstatus( ( pgconn = PQconnectdb( connstr ) ) ) != CONNECTION_OK ) {
			const char fmt[] = "Couldn't initialize PostsreSQL connection: %s";
			snprintf( err, errlen, fmt, PQerrorMessage( pgconn ) );
			PQfinish( pgconn );
			return 0;
		}

		// Run the query
		if ( !( pgres = PQexec( pgconn, query ) ) ) {
			const char fmt[] = "Failed to run query against selected db and table: '%s'";
			snprintf( err, errlen, fmt, PQerrorMessage( pgconn ) );
			//PQclear( res );
			PQfinish( pgconn );
			return 0;
		}

		// Set both of these
		conn->conn = pgconn;
		conn->res = pgres;
	}
#endif

	return 1;
}



typedef struct coerce_t {
	const char name[ 128 ]; 
	const char typename[ 32 ];
} coerce_t;


static const char * find_ctype ( coerce_t **list, const char *name ) {
	for ( coerce_t **x = list; x && *x; x++ ) {
		if ( !strcasecmp( (*x)->name, name ) ) return (*x)->typename;
	}	
	return NULL;
}


/**
 * create_coerced_type( const char *name, const char *typename )
 * ===========================
 *
 * Create an entry
 * 
 */
static coerce_t * create_ctype( const char *src, int len ) {

	coerce_t *t = NULL;
	zw_t pp = { 0 };

	// Stop if there are too many
	if ( memchrocc( src, '=', len ) > 1 ) {
		return NULL;
	}

	// Try allocation
	if ( !( t = malloc( sizeof( coerce_t ) ) ) || !memset( t, 0, sizeof( coerce_t ) ) ) {
		return NULL;
	}

	for ( int i = 0; memwalk( &pp, (unsigned char *)src, (unsigned char *)"=", len, 1 ); i++ ) {
#if 0
fprintf( stderr, "COERCION MULT: " ), 
write( 2, &src[ pp.pos ], pp.size - 1 ), 
fprintf( stderr, "\n" );
#endif
		if ( i == 0 )
			memcpy( ( char *)t->name, &src[ pp.pos ], pp.size - 1 );
		else {
			memcpy( (char *)t->typename, &src[ pp.pos ], pp.size - 1 );
		}
	}

	return t;
}


/**
 * headers_from_dsn( dsn_t * )
 * ===========================
 *
 * Get the headers from a data source.
 * 
 */
int headers_from_dsn ( dsn_t *iconn, char *err, int errlen ) {

	// Define 
	int fcount = 0;
	char *str = NULL;
	char *ename = NULL;
	coerce_t **ctypes = NULL;	
	int ctypeslen = 0;
	const typemap_t *cdefs = NULL;

	// Empty condition to assist with compiling out code 
	if ( 0 ) {
		;
	}
#ifdef BSQLITE_H
	else if ( iconn->type == DB_SQLITE ) {
		snprintf( err, errlen, "%s", "SQLite3 driver not done yet" );
		return 0;
	}
#endif

#ifdef BMYSQL_H
	else if ( iconn->type == DB_MYSQL ) {
		for ( int i = 0, fcount = mysql_field_count( iconn->conn ); i < fcount; i++ ) {
			MYSQL_FIELD *f = mysql_fetch_field_direct( iconn->res, i ); 	
			typemap_t *type = NULL;
			const char *ctype = NULL;
	
			// Allocate a new header_t	
			header_t *st = malloc( sizeof( header_t ) );
			if ( !st || !memset( st, 0, sizeof( header_t ) ) ) {
				const char fmt[] = "Memory constraints encountered while fetching headers from DSN %s";
				snprintf( err, errlen, fmt, iconn->connstr ); 
				return 0;	
			}

			// Get the actual type from the database engine
			st->ntype = f->type;

#if 0
			// Get the type of the column from our list and make sure we can support it
			if ( !( type = get_typemap( mysql_map, st->ntype ) ) ) {
				// TODO: Get the string representation of this name so the user knows
				snprintf( err, errlen, "Invalid MySQL type received: %d", st->ntype );
				return 0;	
			}

			// If the user asked for a coerced type, add it here
			if ( ov && ( ctype = find_ctype( ctypes, f->name ) ) ) {
				// Find the forced/coerced type or die trying
				if ( !( st->ctype = get_typemap_by_native_name( cdefs, ctype ) ) ) {
					const char fmt[] = "Failed to find desired type '%s' for column '%s' in supported %s types";
					snprintf( err, errlen, fmt, ctype, f->name, ename );
					return 0;	
				}
			}

			// Set the base type and write the column name
			st->type = type->basetype;
#endif
		#if 0
			st->maxlen = 0;
			st->precision = 0;
			st->filter = 0;
			st->date = 0; // The timezone data?
		#endif
			snprintf( st->label, sizeof( st->label ), "%s", f->name );	
			add_item( &iconn->headers, st, header_t *, &iconn->hlen ); 
		}
	}
#endif

#ifdef BPGSQL_H
	else if ( iconn->type == DB_POSTGRESQL ) {
		for ( int i = 0, fcount = PQnfields( iconn->res ); i < fcount; i++ ) {
			typemap_t *type = NULL;
			const char *ctype = NULL;
			const char *name = PQfname( iconn->res, i );

			// Allocate a new header_t	
			header_t *st = malloc( sizeof( header_t ) );
			if ( !st || !memset( st, 0, sizeof( header_t ) ) ) {
				fprintf( stderr, "Memory constraints encountered while building schema" );
				return 0;	
			}

			// Get the actual type from the database engine
			st->ntype = PQftype( iconn->res, i );

#if 0 
			// Get the type of the column
			if ( !( type = get_typemap( pgsql_map, st->ntype ) ) ) {
				// TODO: Get the string representation of this name so the user knows
				snprintf( err, errlen, "Invalid PostgreSQL type received: %d", st->ntype );
				return 0;	
			}

			// If the user asked for a coerced type, add it here
			if ( ov && ( ctype = find_ctype( ctypes, name ) ) ) {
				// Find the forced/coerced type or die trying
				if ( !( st->ctype = get_typemap_by_native_name( cdefs, ctype ) ) ) {
					// LEt uesr know chosen engine, and real type name
					const char fmt[] = "Failed to find desired type '%s' for column '%s' in supported %s types";
					snprintf( err, errlen, fmt, ctype, name, ename );
					return 0;	
				}
			}

			// Set the base type and write the column name
			st->type = type->basetype;
#endif
		#if 0
			st->maxlen = 0;
			st->precision = 0;
			st->filter = 0;
			st->date = 0; // The timezone data?
		#endif
			snprintf( st->label, sizeof( st->label ), "%s", name );
			add_item( &iconn->headers, st, header_t *, &iconn->hlen ); 
		}

	}
#endif
	else { 
		// TODO: Reimplement this to read line-by-line
		zw_t p;

		// Prepare this structure
		memset( &p, 0, sizeof( zw_t ) );

		// Copy the delimiter to whatever this is
		memcpy( &delset[ strlen( delset ) ], DELIM, strlen( DELIM ) );

		// Walk through and find each column first
		for ( ; strwalk( &p, iconn->conn, delset ); ) {

			// Define
			char *t = ( !p.size ) ? no_header : copy( &((char *)iconn->conn)[ p.pos ], p.size - 1, case_type );
			header_t *st = malloc( sizeof( header_t ) );

			// ...	
			if ( !st || !memset( st, 0, sizeof( header_t ) ) ) {
				const char fmt[] = 
					"Memory constraints encountered while fetching headers from file '%s'"; 
				snprintf( err, errlen, fmt, iconn->connstr );
				return 0;	
			}
		
			snprintf( st->label, sizeof( st->label ) - 1, "%s", t );
			add_item( &iconn->headers, st, header_t *, &iconn->hlen );
			free( t );
			if ( p.chr == '\n' || p.chr == '\r' ) break;
		}

#if 0
		// If we need to maintain some kind of type safety, run this
		if ( typesafe ) {
			if ( !( str = extract_row( iconn->conn, iconn->clen ) ) ) {
				snprintf( err, errlen, "Failed to extract first row from file '%s'", 
					iconn->connstr ); 
				return 0;	
			}

			// You should do a count really quick on the second rows, or possibly even all rows to make sure that the rows match
			int count = memstrocc( str, &delset[2], strlen( str ) ); 

			// If these don't match, we have a problem
			// Increase by one, since the column count should match the occurrence of delimiters + 1
			if ( iconn->hlen != ++count ) {
				//const char fmt[] = "";
				snprintf( err, errlen, 
					"Header count and column count differ in file: %s (%d != %d)", 
					iconn->connstr,
					iconn->hlen,
					count
				); 
				return 0;	
			}

			// -
			memset( &p, 0, sizeof( zw_t ) );

			// -
			for ( int i = 0, len = 0; strwalk( &p, str, delset ); i++ ) {
				char *t = NULL;
				//typemap_t *type = NULL;
				const char *ctype = NULL;
				const char *name = (iconn->headers[i])->label;
				header_t *st = iconn->headers[ i ];

				// Check if any of these are looking for a coerced type 
				if ( ov && ( ctype = find_ctype( ctypes, name ) ) ) {
					if ( !( st->ctype = get_typemap_by_native_name( cdefs, ctype ) ) ) {
						const char fmt[] = "Failed to find desired type '%s' at column '%s' for supported %s types";
						snprintf( err, errlen, fmt, ctype, name, ename );
						return 0;	
					}
				}

				// Handle NULL or empty strings
				if ( !p.size ) {
					st->type = T_STRING;
					continue;
				}

				// Handle NULL or empty strings
				t = copy( &str[ p.pos ], p.size - 1, 0 );
				if ( ( len = strlen( t ) ) == 0 ) {
					st->type = T_STRING;
					continue;
				}

				st->type = get_type( t, T_STRING ); 

				// TODO: This should be a static buffer
				free( t );
			}

			// TODO: This should be a static buffer
			free( str );
		}
#endif
	}

	return 1;
}





/**
 * struct_from_dsn( dsn_t * )
 * ===========================
 *
 * Create a struct from data source.
 * 
 */
int struct_from_dsn( dsn_t * conn ) {

	char **defs = NULL;
	if ( classdata ) {
		defs = class_coldefs; 
		fprintf( stdout, "class %s {\n", root );
	}
	else {
		defs = struct_coldefs; 
		fprintf( stdout, "struct %s {\n", root );
	}

	// Add the rest of the rows
	for ( header_t **s = conn->headers; s && *s; s++ ) {
		fprintf( stdout, "\t%s %s;\n", defs[ (*s)->type ], (*s)->label );
	}

	return 1;
}



/**
 * prepare_dsn( dsn_t * )
 * ===========================
 *
 * Prepare DSN for reading.  If we processed a limit, that would
 * really be the only way to make this work sensibly with SQL.
 * 
 */
int prepare_dsn ( dsn_t *conn, char *err, int errlen ) {
	if ( conn->type == DB_FILE ) {
		//while ( *((char *)conn->conn++) != '\n' );
	}
	return 1;
}



/**
 * records_from_dsn( dsn_t * )
 * ===========================
 *
 * Extract records from a data source.
 * 
 */
int records_from_dsn( dsn_t *conn, int count, int offset, char *err, int errlen ) {

	if ( conn->type == DB_FILE ) {
		zw_t *p = malloc( sizeof( zw_t ) );
		memset( p, 0, sizeof( zw_t ) );
		column_t **cols = NULL;
		row_t *row = NULL;

		//TODO: this will have to be rewritten to handle buffering
		char *buffer = conn->conn;

		// Move up the buffer so that we start from the right offset	
		while ( *(buffer++) != '\n' );
	
		//
		for ( int line = 0, ci = 0, clen = 0; strwalk( p, buffer, delset ); line++ ) {
			//snprintf( err, errlen, "%d, %d\n", p.chr, *p.ptr );

			// Allocate, prepare and set column values
			column_t *col = NULL;

			// Increase the line count
			if ( p->chr == '\r' ) {	
				line++;
				continue;
			}

			// Exit if the column count indicates that this is not uniform data
			if ( ci > conn->hlen ) {
				//( !rows ) ? free_inner_records( columns ) : free_records( rows );
				const char fmt[] = "Column count (%d) does not match header count (%d) at line %d in file '%s'";
				snprintf( err, errlen, fmt, ci, conn->hlen, line, conn->connstr );
				return 0;
			}

			// Allocate, prepare and set column values
			if ( !( col = malloc( sizeof( column_t ) ) ) || !memset( col, 0, sizeof( column_t ) ) ) {
				const char fmt[] = "Out of memory when allocating space for value at line %d, column %d: %s";
				snprintf( err, errlen, fmt, line, ci, strerror( errno ) );
				return 0;
			}

	#if 1
			// Set any remaining column values
			if ( !( col->v = malloc( p->size + 1 ) ) || !memset( col->v, 0, p->size + 1 ) ) {
				const char fmt[] = "Out of memory when allocating space for value at line %d, column %d: %s";
				snprintf( err, errlen, fmt, line, ci, strerror( errno ) );
				return 0;
			}

			// Set the header ref and actual column value
			extract_value_from_column( &( (char *)buffer)[ p->pos ], (char **)&col->v, p->size - 1 );
	#else
			col->v = getvalue( &( (unsigned char *)conn->conn )[ p->pos ], p->size + 1 );  
	#endif
			col->len = p->size;
			col->k = conn->headers[ ci ]->label;

		#if 1
			// Check the type of value
			if ( typesafe ) {
				// The expected type SHOULD be in the headers
				col->exptype = conn->headers[ ci ]->type;
				
				// Get the real type
				col->type = get_type( ( char * )col->v, col->exptype );

				//
				if ( typesafe && col->exptype != col->type ) {
					const char fmt[] = "Type check for value at column '%s', line %d failed.\n";
					snprintf( err, errlen, fmt, col->k, line );
					return 0;
				}
			}
		#endif

			// Add columns
			add_item( &cols, col, column_t *, &clen );

			// Increase the column index
			ci++;

			// Increment line
			if ( p->chr == '\n' ) {
				// Allocate one row
				if ( !( row = malloc( sizeof( row_t ) ) ) ) {
					const char fmt[] = "Out of memory when allocating space for new row at line %d: %s"; 
					snprintf( err, errlen, fmt, line, strerror( errno ) );
					return 0;
				}

				// Add an item to said row
				row->columns = cols;

				// And add this row to rows
				add_item( &conn->rows, row, row_t *, &conn->rlen );	

				// Reset everything
				cols = NULL, clen = 0, ci = 0, line++;
			}
		
		}

		free( p );

	//This ensures that the final row is added to list...
	//TODO: It'd be nice to have this within the abrowse loop.
	//add_item( &rows, columns, record_t **, &ol );	
	}
#ifdef BSQLITE_H
	else if ( conn->type == DB_SQLITE ) {
		;
	}
#endif
#ifdef BMYSQL_H
	else if ( conn->type == DB_MYSQL ) {
	#if 1
		my_ulonglong rowcount = mysql_num_rows( conn->res );
	#else
		//MYSQL_RES *res = NULL; 
		int blimit = 1000;
		my_ulonglong rowcount = mysql_num_rows( conn->res );
		int loopcount = ( rowcount / blimit < 1 ) ? 1 : rowcount / blimit;
		if ( rowcount % blimit && loopcount > 1 ) {
			loopcount++;
		}

		// Move through all of the rows
		// for ( int lc = 0, rc = 0; lc < loopcount; lc++ ) {
	#endif

		// Move through all of the columns
		for ( int i = 0; i < rowcount ; i++ ) {
			MYSQL_ROW r = mysql_fetch_row( conn->res );
			row_t *row = NULL;
			column_t **cols = NULL;
			unsigned long *lens = mysql_fetch_lengths( conn->res );

			// Move through each column
			for ( int ci = 0, clen = 0; ci < conn->hlen; ci++ ) {
				// Define
				column_t *col = malloc( sizeof( column_t ) ); 

				// Check for alloc failure
				if ( !col || !memset( col, 0, sizeof( column_t ) ) ) {
					const char fmt[] = "Out of memory when allocating space for entry at row %d, column %d: %s"; 
					snprintf( err, errlen, fmt, i, ci, strerror( errno ) );
					return 0;		
				}

				// Set the values
				col->k = conn->headers[ ci ]->label; 
				col->type = conn->headers[ ci ]->type; 

				// Get the value and trim stuff				
				// TODO: Using trim(...) here might be dangerous and lead to leaks...
				col->v = (unsigned char *)( *r );

				// This SHOULD be fine even if we're dealing with strings
				col->len = lens[ ci ];

				// Add to the column set
				add_item( &cols, col, column_t *, &clen );

				// This can be a debug only message
				const char fmt[] = "Fetching value %p at column '%s' of type '%s' with length = %d\n";
				fprintf( stderr, fmt, (void *)*r, col->k, itypes[ col->type ], col->len ); 
				r++;
			}

			// Add a new row
			if ( !( row = malloc( sizeof( row_t ) ) ) ) {
				const char fmt[] = "Out of memory when allocating space for row %d at cursor: %s"; 
				snprintf( err, errlen, fmt, i, strerror( errno ) );
				return 0;
			}

			// Add an item to said row
			row->columns = cols;

			// And add this row to rows
			add_item( &conn->rows, row, row_t *, &conn->rlen );	

			// Reset everything
			cols = NULL;
		}
		//}
	}
#endif
#ifdef BPGSQL_H
	else if ( conn->type == DB_POSTGRESQL )  {
		int rcount = PQntuples( conn->res );
		for ( int i = 0; i < rcount; i++ ) {
			row_t *row = NULL;
			column_t **cols = NULL;

			for ( int ci = 0, clen = 0; ci < conn->hlen; ci++ ) {
				// Define
				column_t *col = malloc( sizeof( column_t ) ); 

				// Check for alloc failure
				if ( !col || !memset( col, 0, sizeof( column_t ) ) ) {
					const char fmt[] = "Out of memory when allocating space for entry at row %d, column %d: %s"; 
					snprintf( err, errlen, fmt, i, ci, strerror( errno ) );
					return 0;		
				}

				// Set the value, column name and more for the record
				col->k = conn->headers[ ci ]->label;
				col->len = PQgetlength( conn->res, i, ci );
				col->type = conn->headers[ ci ]->type; 
				col->v = (unsigned char *)PQgetvalue( conn->res, i, ci );
				//col->v = (unsigned char *)strdup( PQgetvalue( conn->res, i, ci ) );

				// Add it
				add_item( &cols, col, column_t *, &clen );	
			}

			// Add a new row
			if ( !( row = malloc( sizeof( row_t ) ) ) ) {
				const char fmt[] = "Out of memory when allocating space for row %d at cursor: %s"; 
				snprintf( err, errlen, fmt, i, strerror( errno ) );
				return 0;
			}

			// Add an item to said row
			row->columns = cols;

			// And add this row to rows
			add_item( &conn->rows, row, row_t *, &conn->rlen );	

			// Reset everything
			cols = NULL;
		}
	}
#endif

	return 1;
}




/**
 * transform_from_dsn( dsn_t * )
 * ===================
 *
 * "Transform" the records into whatever output format you're looking for.
 * 
 */
int transform_from_dsn( 
	dsn_t *iconn, 
	dsn_t *oconn, 
	stream_t t, 
	int count, 
	int offset, 
	char *err,
	int errlen
) {

	// Define
	int odv = 0;
	char insfmt[ 256 ];
	memset( insfmt, 0, sizeof( insfmt ) );

	// Define any db specific stuff
	int ri = 0;	
#ifdef BMYSQL_H
	MYSQL_STMT *mysql_stmt = NULL;
	MYSQL_BIND *mysql_bind_array = NULL;
	MYSQL_BIND ms_binds[ iconn->hlen ];
#endif
#ifdef BPGSQL_H
	char **pgsql_bind_array = NULL;
	int *pgsql_lengths_array = 0;
#endif

	// Make the thing
	if ( 0 );
#ifdef BMYSQL_H
	else if ( oconn->type == DB_MYSQL ) {
		mysql_stmt = (void *)mysql_stmt_init( oconn->conn );
#if 1
		mysql_bind_array = malloc( sizeof( MYSQL_BIND ) * iconn->hlen );
		memset( mysql_bind_array, 0, sizeof( MYSQL_BIND ) * iconn->hlen );
#endif
		memset( &ms_binds, 0, iconn->hlen );
	}
#endif
#ifdef BPGSQL_H
	else if ( oconn->type == DB_POSTGRESQL ) {
		pgsql_bind_array = malloc( sizeof( char * ) * iconn->hlen );
		memset( pgsql_bind_array, 0, sizeof( char * ) * iconn->hlen );
		pgsql_lengths_array = malloc( sizeof( int ) * iconn->hlen );
		memset( pgsql_lengths_array, 0, sizeof( int ) * iconn->hlen ); 
	#if 0
		pgsql_types_array = malloc( sizeof( int ) * iconn->hlen );
		memset( pgsql_types_array, 0, sizeof( int ) * iconn->hlen ); 
	#endif
	}
#endif

	// Loop through all rows
	for ( row_t **row = iconn->rows; row && *row; row++, ri++ ) {

		// A buffer for strings
		char ffmt[ MAX_STMT_SIZE ];
		memset( ffmt, 0, MAX_STMT_SIZE );

		//Prefix
		if ( prefix ) {
			fprintf( oconn->output, "%s", prefix );
		}

		if ( t == STREAM_PRINTF )
			;//p_default( 0, ib->k, ib->v );
		else if ( t == STREAM_XML )
			( prefix && newline ) ? fprintf( oconn->output, "%c", '\n' ) : 0;
		else if ( t == STREAM_COMMA )
			;//fprintf( oconn->output, "" );
		else if ( t == STREAM_CSTRUCT )
			fprintf( oconn->output, "%s{", odv ? "," : " "  );
		else if ( t == STREAM_CARRAY )
			fprintf( oconn->output, "%s{ ", odv ? "," : " "  ); 
		else if ( t == STREAM_JSON ) {
		#if 1
			fprintf( oconn->output, "%c{%s", odv ? ',' : ' ', newline ? "\n" : "" );
		#else
			if ( prefix && newline )
				fprintf( oconn->output, "%c{\n", odv ? ',' : ' ' );
			else if ( prefix ) 
				;
			else {
				fprintf( oconn->output, "%c{%s", odv ? ',' : ' ', newline ? "\n" : "" );
			}
		#endif
		}
		else if ( t == STREAM_SQL ) {
			// Output the table name
			fprintf( oconn->output, "INSERT INTO %s ( ", iconn->tablename );

			for ( header_t **x = iconn->headers; x && *x; x++ ) {	
				// TODO: use the pointer to detect whether or not we're at the beginning
				fprintf( oconn->output, &",%s"[ ( *iconn->headers == *x ) ], (*x)->label );
			}
	
			//The VAST majority of engines will be handle specifying the column names 
			fprintf( oconn->output, " ) VALUES ( " );
		}
		//else if ( t == STREAM_PGSQL || t == STREAM_MYSQL /* t == STREAM_SQLITE3 */ ) {
	#ifdef BPGSQL_H
		else if ( t == STREAM_PGSQL ) {
			// First (and preferably only once) generate the header keys
			char argfmt[ 256 ];
			char *ins = insfmt;
			int ilen = sizeof( insfmt );
			char *s = argfmt;

			// Initialize
			memset( insfmt, 0, sizeof( insfmt ) );
			memset( argfmt, 0, sizeof( argfmt ) );

			// Reset the values counter
			//vbindlen = 0;	

			// Create the header keys 
			for ( header_t **h = iconn->headers; h && *h; h++ ) {
				int i = snprintf( ins, ilen, &",%s"[ ( *iconn->headers == *h ) ], (*h)->label );
				ins += i, ilen -= i;
			} 	

			// Create the insert keys
			for ( unsigned int bw = 0, i = 1, l = sizeof( argfmt ); i <= iconn->hlen && l; i++ ) {
				// TODO: You're going to need a way to track how big len is
				//int bw = snprintf( s, l, &",:%d"[ (int)(i == 0) ] , i );
				bw = snprintf( s, l, &",$%d"[ (int)(i == 1) ], i );
				l -= bw, s += bw;
			}

			// Finally, we create the final bind stmt
			snprintf( ffmt, sizeof( ffmt ), "INSERT INTO %s ( %s ) VALUES ( %s )", iconn->tablename, insfmt, argfmt );
			fprintf( stdout, "%s\n", ffmt );
		}
	#endif
	#ifdef BMYSQL_H
		else if ( t == STREAM_MYSQL /* t == STREAM_SQLITE3 */ ) {
			// First (and preferably only once) generate the header keys
			char argfmt[ 256 ];
			char *ins = insfmt;
			int ilen = sizeof( insfmt );
			char *s = argfmt;

			// Initialize
			memset( insfmt, 0, sizeof( insfmt ) );
			memset( argfmt, 0, sizeof( argfmt ) );

			// Reset the values counter
			//vbindlen = 0;	

			// Create the header keys 
			for ( header_t **h = iconn->headers; h && *h; h++ ) {
				int i = snprintf( ins, ilen, &",%s"[ ( *iconn->headers == *h ) ], (*h)->label );
				ins += i, ilen -= i;
			} 	

			// Create the insert keys
			for ( unsigned int bw = 0, i = 1, l = sizeof( argfmt ); i <= iconn->hlen && l; i++ ) {
				// TODO: You're going to need a way to track how big len is
				//int bw = snprintf( s, l, &",:%d"[ (int)(i == 0) ] , i );
				bw = snprintf( s, l, &",%s"[ (int)(i == 1) ], "?" );
				
				l -= bw, s += bw;
			}

			// Finally, we create the final bind stmt
			snprintf( ffmt, sizeof( ffmt ), "INSERT INTO %s ( %s ) VALUES ( %s )", iconn->tablename, insfmt, argfmt );
			fprintf( stdout, "%s\n", ffmt );
		}
	#endif


//fprintf( stdout, "%s\n", ffmt );
//continue;


		// Define
		int ci = 0;

		// Loop through each column
		for ( column_t **col = (*row)->columns; col && *col; col++, ci++ ) {
			//Sadly, you'll have to use a buffer...
			//Or, use hlen, since that should contain a count of columns...
			int first = *col == *((*row)->columns); 
			if ( t == STREAM_PRINTF )
				fprintf( oconn->output, "%s => %s%s%s\n", (*col)->k, ld, (*col)->v, rd );
			else if ( t == STREAM_XML )
				fprintf( oconn->output, "%s<%s>%s</%s>\n", &TAB[9-1], (*col)->k, (*col)->v, (*col)->k );
			else if ( t == STREAM_CARRAY )
				fprintf( oconn->output, &", \"%s\""[ first ], (*col)->v );
			else if ( t == STREAM_COMMA )
				fprintf( oconn->output, &",\"%s\""[ first ], (*col)->v );
			else if ( t == STREAM_JSON ) {
				fprintf( oconn->output, "\t%c\"%s\": ", first ? ' ' : ',', (*col)->k ); 
				if ( (*col)->type == T_STRING || (*col)->type == T_CHAR )
					fprintf( oconn->output, "\"%s\"\n", (*col)->v );
				else if ( (*col)->type == T_NULL ) 
					fprintf( oconn->output, "null\n" );
				#if 0
				else if ( (*col)->type == T_BOOLEAN ) {
					fprintf( oconn->output, "%s\n", ( *col->v ) ? "true" : "false" );
				}
				#endif	
				else {
					fprintf( oconn->output, "%s\n", (*col)->v );
				}
			}
			else if ( t == STREAM_CSTRUCT ) {
				//p_cstruct( 1, *col );
				//check that it's a number
				char *vv = (char *)(*col)->v;

				//Handle blank values
				if ( !strlen( vv ) ) {
					fprintf( oconn->output, "%s", &TAB[9-1] );
					fprintf( oconn->output, &", .%s = \"\"\n"[ first ], (*col)->k );
				}
				else {
					fprintf( oconn->output, "%s", &TAB[9-1] );
					fprintf( oconn->output, &", .%s = \"%s\"\n"[ first ], (*col)->k, (*col)->v );
				}
			}
			else if ( t == STREAM_SQL ) {
				if ( !typesafe ) {
					fprintf( oconn->output, &",%s%s%s"[ first ], ld, (*col)->v, rd );
				}
				else {
					// Should check that they match too
					if ( (*col)->type != T_STRING && (*col)->type != T_CHAR )
						fprintf( oconn->output, &",%s"[ first ], (*col)->v );
					else {
						fprintf( oconn->output, &",%s%s%s"[ first ], ld, (*col)->v, rd );
					}
				}
			}
		#ifdef BMYSQL_H
			else if ( t == STREAM_MYSQL ) {
			#ifdef DEBUG_H
				// We definitely do use this...
				header_t **headers = iconn->headers;
				fprintf( stderr, "Binding value %p, %d of length %d\n", (*col)->v, (headers[ci])->ntype,(*col)->len );
			#endif

			#if 1
				MYSQL_BIND *bind = &mysql_bind_array[ ci ];
				bind->buffer = (*col)->v;
				bind->is_null = NULL;
				bind->buffer_length = (*col)->len;
				// Use the approximate type as a base for the format
				if ( (*col)->type == T_NULL )
					bind->buffer_type = MYSQL_TYPE_STRING;
				else if ( (*col)->type == T_BOOLEAN )
					bind->buffer_type = MYSQL_TYPE_BIT;
				else if ( (*col)->type == T_STRING || (*col)->type == T_CHAR )
					bind->buffer_type = MYSQL_TYPE_STRING;
				else if ( (*col)->type == T_INTEGER )
					bind->buffer_type = MYSQL_TYPE_DECIMAL;
				else if ( (*col)->type == T_DOUBLE )
					bind->buffer_type = MYSQL_TYPE_DOUBLE;
			#if 0
				else if ( (*col).type == T_BINARY ) {
				//binds[ ci ].buffer_type = (headers[ ci ])->ntype;
				}
			#endif
				else {
					// We ought to not get here...
					const char fmt[] = "Invalid bind type";
					snprintf( err, errlen, fmt );
					return 0;	
				}
			#else
			#endif
			}
		#endif
		#ifdef BPGSQL_H
			else if ( t == STREAM_PGSQL ) {
			#ifdef DEBUG_H
				fprintf( stderr, "Binding value %p of length %d\n", (*col)->v, (*col)->len );
			#endif
				// Bind and prepare an insert?		
				pgsql_bind_array[ ci ] = (char *)(*col)->v; 
				pgsql_lengths_array[ ci ] = (*col)->len; 
			#if 0
				// Use the approximate type as a base for the format
				if ( (*col)->type == T_NULL )
					pgsql_types_array[ ci ].buffer_type = MYSQL_TYPE_STRING;
				else if ( (*col)->type == T_BOOLEAN )
					pgsql_types_array[ ci ].buffer_type = MYSQL_TYPE_BIT;
				else if ( (*col)->type == T_STRING || (*col)->type == T_CHAR )
					pgsql_types_array[ ci ].buffer_type = MYSQL_TYPE_STRING;
				else if ( (*col)->type == T_INTEGER )
					pgsql_types_array[ ci ].buffer_type = MYSQL_TYPE_DECIMAL;
				else if ( (*col)->type == T_DOUBLE )
					pgsql_types_array[ ci ].buffer_type = MYSQL_TYPE_DOUBLE;
				else if ( (*col)->type == T_DOUBLE ) {
					//pgsql_types_array[ ci ].buffer_type = MYSQL_TYPE_DOUBLE;
				}
				else {
					// We ought to not get here...
					const char fmt[] = "Invalid bind type";
					snprintf( err, errlen, fmt );
					return 0;	
				}
			#endif
			}
		#endif
		#if 0
			else if ( t == STREAM_SQLITE ) {
			}
		#endif
		} // end for

		// Third, generate a full string
		// We'll have to actually commit the row here
		// Or use a transaction to save some time...

		//Built-in suffix...
		if ( t == STREAM_PRINTF )
			;//p_default( 0, ib->k, ib->v );
		else if ( t == STREAM_XML )
			;//p_xml( 0, ib->k, ib->v );
		else if ( t == STREAM_COMMA )
			;//fprintf( oconn->output, " },\n" );
		else if ( t == STREAM_CSTRUCT )
			fprintf( oconn->output, "}" );
		else if ( t == STREAM_CARRAY )
			fprintf( oconn->output, " }" );
		else if ( t == STREAM_SQL )
			fprintf( oconn->output, " );" );
		else if ( t == STREAM_JSON ) {
			//wipe the last ','
			fprintf( oconn->output, "}" );
			//fprintf( oconn->output, "," );
		}

	#ifdef BMYSQL_H
		else if ( t == STREAM_MYSQL ) {
		#ifdef DEBUG_H
			fprintf( stderr, "Writing values to MySQL db\n" );
		#endif
		#if 0
			int status = 0;
			status = mysql_bind_param( oconn->conn, iconn->hlen, mysql_bind_array, NULL );
			if ( status != 0 ) {
				const char fmt[] = "MySQL bind failed: '%s'";
				snprintf( err, errlen, fmt, mysql_error( oconn->conn )  ); 
				return 0;	
			}

			status = mysql_real_query( oconn->conn, ffmt, strlen( ffmt ) );
			if ( status != 0 ) {
				const char fmt[] = "MySQL real query failed: '%s'";
				snprintf( err, errlen, fmt, mysql_error( oconn->conn )  ); 
				return 0;	
			}
		#else
			// Prepare the statement
			if ( mysql_stmt_prepare( mysql_stmt, ffmt, strlen( ffmt ) ) != 0 ) {
				const char fmt[] = "MySQL statement prepare failure: '%s'";
				snprintf( err, errlen, fmt, mysql_stmt_error( mysql_stmt )  ); 
				return 0;	
			}

			// Use the parameter count to check that everything worked (probably useless)
			if ( mysql_stmt_param_count( mysql_stmt ) != iconn->hlen ) {
				snprintf( err, errlen, "MySQL column header count mismatch." );
				return 0;	
			}

			if ( mysql_stmt_bind_param( mysql_stmt, mysql_bind_array ) != 0 ) {
				const char fmt[] = "MySQL bind failure: '%s'";
				snprintf( err, errlen, fmt, mysql_stmt_error( mysql_stmt )  ); 
				return 0;	
			}

			if ( mysql_stmt_execute( mysql_stmt ) != 0 ) {
				const char fmt[] = "MySQL statement exec failure: '%s'";
				snprintf( err, errlen, fmt, mysql_stmt_error( mysql_stmt ) ); 
				return 0;	
			}
		#endif
		}
	#endif

	#ifdef BPGSQL_H
		else if ( t == STREAM_PGSQL ) {
		#ifdef DEBUG_H
			fprintf( stderr, "Writing values to Postgres db\n" );
		#endif

			PGresult *r = PQexecParams( 	
				oconn->conn, 
				ffmt, 
				iconn->hlen,
				NULL,
				(const char * const *)pgsql_bind_array,
				pgsql_lengths_array,
				NULL,
				0	
			);
	
			if ( PQresultStatus( r ) != PGRES_COMMAND_OK ) {
				const char fmt[] = "PostgreSQL commit failed: %s";
				snprintf( err, errlen, fmt, PQresultErrorMessage( r ) ); 
				return 0;
			}

			PQclear( r );
		}
	#endif

//fprintf( stdout, "Completed row: %d", ri ), fflush( stdout ), getchar();

		//Suffix
		if ( suffix ) {
			fprintf( oconn->output, "%s", suffix );
		}

		if ( newline ) {
			//fprintf( oconn->output, "%s", NEWLINE );
		}

		// If the stream is a file, flush it 
		if ( t != STREAM_MYSQL && t != STREAM_PGSQL ) {
			fflush( oconn->output );
		}	
	}

	if ( 0 ) ;
#ifdef BMYSQL_H
	else if ( oconn->type == DB_MYSQL ) {
		mysql_stmt_close( mysql_stmt );
		free( mysql_bind_array ); 
	}
#endif
#ifdef BPGSQL_H
	else if ( oconn->type == DB_POSTGRESQL ) {
		free( pgsql_bind_array ); 
		free( pgsql_lengths_array ); 
	}
#endif


	return 1;
}



/**
 * close_dsn( dsn_t * )
 * ===================
 *
 * Close a data source.
 * 
 */
void close_dsn( dsn_t *conn ) {

	DPRINTF( "Attempting to close connection at %p\n", (void *)conn->conn );

	if ( conn->type == DB_FILE )
		free( conn->conn );
#ifdef BSQLITE_H
	else if ( conn->type == DB_SQLITE )
		fprintf( stderr, "SQLite3 not done yet.\n" );
#endif
#ifdef BMYSQL_H
	else if ( conn->type == DB_MYSQL ) {
		mysql_free_result( conn->res );
		mysql_close( (MYSQL *)conn->conn );
	}
#endif
#ifdef BPGSQL_H
	else if ( conn->type == DB_POSTGRESQL ) {
		PQclear( conn->res );
		PQfinish( (PGconn *)conn->conn );
	}
#endif

	free( conn->connstr );
	conn->conn = NULL;
}


// Determine DSN backend
int dsn_engine_stuff( dsn_t *iconn, dsn_t *oconn ) {
	return 0;
}


// Determine the relationship between in & out srcs
int rels_between_dsn( dsn_t *iconn, dsn_t *oconn, const char *err, int errlen ) {

	// If the user was specific about which engine to use, use those settings
	// If not, then use the defaults
	// Also, if the type is not available, we should warn and say something
	if ( iconn->type == DB_FILE ) {
		// Set up the backend	
		if ( oconn->type == DB_SQLITE )
			oconn->typemap = default_map;
		else if ( oconn->type == DB_MYSQL )
			oconn->typemap = mysql_map;
		else if ( oconn->type == DB_POSTGRESQL )
			oconn->typemap = pgsql_map;
	}
	else if ( iconn->type == DB_SQLITE ) {
		oconn->typemap = default_map;
		//oconn->convmap = 
	}
	else if ( iconn->type == DB_MYSQL ) {
		oconn->typemap = mysql_map;
		//oconn->convmap = 
	}
	else if ( iconn->type == DB_POSTGRESQL ) {
		oconn->typemap = pgsql_map;
	}

	return 1;
}


/**
 * types_from_dsn( dsn_t * )
 * ===========================
 *
 * Checks that we can support all of the types specified by a query
 * 
 */
int types_from_dsn( dsn_t * iconn, dsn_t *oconn, char *ov, char *err, int errlen ) {
	// Define 
	int fcount = 0;
	char *str = NULL;
	char *ename = NULL;
	coerce_t **ctypes = NULL;	
	int ctypeslen = 0;
	const typemap_t *cdefs = NULL;

#if 1
	// Evaluate the coercion string if one is available
	// The type that is saved will be the coerced one
	if ( ov ) {
		// Multiple ovs
		if ( memchr( ov, ',', strlen( ov ) ) ) {
			zw_t p = { 0 };
			for ( ; strwalk( &p, ov, "," ); ) {
				coerce_t *t = NULL;
				if ( ( t = create_ctype( &ov[ p.pos ], p.size ) ) ) {
					add_item( &ctypes, t, coerce_t *, &ctypeslen );
				}
			}
		}

		// Single coerce	
		else if ( memchr( ov, '=', strlen( ov ) ) ) {
			coerce_t *t = NULL;
			if ( ( t = create_ctype( ov, strlen( ov ) ) ) ) {
				add_item( &ctypes, t, coerce_t *, &ctypeslen );
			}
		}

		// Something weird...
		else {
			snprintf( err, errlen, "Argument supplied to --coerce is invalid.\n" );	
			return 0;
		}
	}
#endif



	// 
	if ( iconn->type == DB_FILE ) { 
		zw_t p;
		int count = 0;

		// Get the SECOND row only and estimate types that way
		if ( !( str = extract_row( iconn->conn, iconn->clen ) ) ) {
			const char fmt[] = "Failed to extract first row from file '%s'";
			snprintf( err, errlen, fmt, iconn->connstr ); 
			return 0;	
		}

		// Check that col count - 1 == occurrence of delimiters in file
		if ( iconn->hlen - 1 != ( count = memstrocc( str, &delset[2], strlen( str ) ) ) ) {
			const char fmt[] = "Header count and column count differ in file: %s (%d != %d)"; 
			snprintf( err, errlen, fmt, iconn->connstr, iconn->hlen, count ); 
			free( str );
			return 0;	
		}

		memset( &p, 0, sizeof( zw_t ) );

		// Move through the rest of the entries in the row and approximate types 
		for ( int i = 0, len = 0; strwalk( &p, str, delset ); i++ ) {
			header_t *st = iconn->headers[ i ];
			char *t = NULL;
			const char *ctype = NULL;
			const char *name = st->label;

			// Check if any of these are looking for a coerced type 
			if ( ov && ( ctype = find_ctype( ctypes, name ) ) ) {
				if ( !( st->ctype = get_typemap_by_native_name( oconn->typemap, ctype ) ) ) {
					const char fmt[] = "Failed to find desired type '%s' at column '%s' for supported %s types";
					snprintf( err, errlen, fmt, ctype, name, get_conn_type( oconn->type ) );
					free( str );
					return 0;	
				}
			}

			// TODO: Assuming a blank value is a string might not always be the best move
			if ( !p.size ) {
				st->type = T_STRING;
				continue;
			}

			// Try to copy again
			if ( !( t = copy( &str[ p.pos ], p.size - 1, 0 ) ) ) {
				// TODO: This isn't very clear, but it is unlikely we'll ever run into it...
				snprintf( err, errlen, "Out of memory error occurred." );
				free( str );
				return 0;
			}

			// Handle NULL or empty strings
			if ( ( len = strlen( t ) ) == 0 ) {
				st->type = T_STRING;
				continue;
			}

			st->type = get_type( t, T_STRING ); 

			// TODO: This should be a static buffer
			free( t );
		}

		free( str );
	}
#ifdef BSQLITE_H
	else if ( iconn->type == DB_SQLITE ) {
		snprintf( err, errlen, "%s", "SQLite3 driver not done yet" );
		return 0;
	}
#endif

#ifdef BMYSQL_H
	else if ( iconn->type == DB_MYSQL ) {
		for ( int i = 0, fcount = mysql_field_count( iconn->conn ); i < fcount; i++ ) {
			MYSQL_FIELD *f = mysql_fetch_field_direct( iconn->res, i ); 	
			header_t *st = iconn->headers[ i ];
			typemap_t *type = NULL;
			const char *ctype = NULL;
	
			// Get the actual type from the database engine
			st->ntype = f->type;

			// Get the type of the column from our list and make sure we can support it
			if ( !( type = get_typemap( iconn->typemap, st->ntype ) ) ) {
				// TODO: Get the string representation of this name so the user knows
				snprintf( err, errlen, "Unsupported MySQL type received: %d", st->ntype );
				return 0;	
			}

			// If the user asked for a coerced type, add it here
			if ( ov && ( ctype = find_ctype( ctypes, f->name ) ) ) {
				// Find the forced/coerced type or die trying
				if ( !( st->ctype = get_typemap_by_native_name( oconn->typemap, ctype ) ) ) {
					const char fmt[] = "Failed to find desired type '%s' for column '%s' in supported %s types";
					snprintf( err, errlen, fmt, ctype, f->name, ename );
					return 0;	
				}
			}

			// Finally, if the output type is a db and different, we'll need to match that type here...

			// Set the base type and write the column name
			st->type = type->basetype;
		#if 0
			st->maxlen = 0;
			st->precision = 0;
			st->filter = 0;
			st->date = 0; // The timezone data?
		#endif
			snprintf( st->label, sizeof( st->label ), "%s", f->name );	
			add_item( &iconn->headers, st, header_t *, &iconn->hlen ); 
		}
	}
#endif

#ifdef BPGSQL_H
	else if ( iconn->type == DB_POSTGRESQL ) {
		for ( int i = 0, fcount = PQnfields( iconn->res ); i < fcount; i++ ) {
			header_t *st = iconn->headers[ i ];
			typemap_t *type = NULL;
			const char *ctype = NULL;
			const char *name = PQfname( iconn->res, i );

			// Get the actual type from the database engine
			st->ntype = PQftype( iconn->res, i );
 
			// Get the type of the column
			if ( !( type = get_typemap( iconn->typemap, st->ntype ) ) ) {
				// TODO: Get the string representation of this name so the user knows
				snprintf( err, errlen, "Invalid PostgreSQL type received: %d", st->ntype );
				return 0;	
			}

			// If the user asked for a coerced type, add it here
			if ( ov && ( ctype = find_ctype( ctypes, name ) ) ) {
				// Find the forced/coerced type or die trying
				if ( !( st->ctype = get_typemap_by_native_name( cdefs, ctype ) ) ) {
					// LEt uesr know chosen engine, and real type name
					const char fmt[] = "Failed to find desired type '%s' for column '%s' in supported %s types";
					snprintf( err, errlen, fmt, ctype, name, ename );
					return 0;	
				}
			}

			// Set the base type and write the column name
			st->type = type->basetype;
		#if 0
			st->maxlen = 0;
			st->precision = 0;
			st->filter = 0;
			st->date = 0; // The timezone data?
		#endif
			snprintf( st->label, sizeof( st->label ), "%s", name );
			add_item( &iconn->headers, st, header_t *, &iconn->hlen ); 
		}

	}
#endif


	for ( coerce_t **x = ctypes; x && *x; x++ ) {
		free( *x );
	}

	free( ctypes );
	return 1;
return 0;
}




//Options
int help () {
	const char *fmt = "%-2s%s --%-24s%-30s\n";
	struct help { const char *sarg, *larg, *desc; } msgs[] = {
		{ "-i", "input <arg>", "Specify an input datasource (required)"  },
		{ "", "create", "If an output source does not exist, create it"  },
		{ "-o", "output <arg>","Specify an output datasource (optional, default is stdout)"  },
		{ "-c", "convert",     "Converts input data to another format or into a datasource" },
		{ "-H", "headers",     "Displays headers of input datasource and stop"  },
		{ "-S", "schema",      "Generates an SQL schema using headers of input datasource" },
		{ "-d", "delimiter <arg>", "Specify a delimiter when using a file as input datasource" },
#if 0
		{ "-r", "root <arg>",  "Specify a $root name for certain types of structures.\n"
			"                              (Example using XML: <$root> <key1></key1> </$root>)" }, 
		{ "",   "name <arg>",  "Synonym for root." },
#endif
		{ "-u", "output-delimiter <arg>", "Specify an output delimiter when generating serialized output" },
	#if 1
		{ "-t", "typesafe",    "Enforce and/or enable typesafety" },
		{ "-C", "coerce",    "Specify a type for a column" },
	#endif
		{ "-f", "format <arg>","Specify a format to convert to" },
		{ "-j", "json",        "Convert into JSON (short for --convert --format \"json\")" },
		{ "-x", "xml",         "Convert into XML (short for --convert --format \"xml\")" },
		{ "-q", "sql",         "Convert into general SQL INSERT statement." },
		{ "-p", "prefix <arg>","Specify a prefix" },
		{ "-s", "suffix <arg>","Specify a suffix" },
		{ "",   "class <arg>", "Generate a Java-like class using headers of input datasource" },
		{ "",   "struct <arg>","Generate a C-style struct using headers of input datasource"  },
		{ "",   "for <arg>",   "Use <arg> as the backend when generating a schema\n"
		  "                              (See `man briggs` for available backends)" },
		{ "",   "camel-case",  "Use camel case for class properties or struct labels" },
#if 0
		{ "",   "snake-case",  "Use camel case for class properties or struct labels" },
#endif
		{ "-n", "no-newline",  "Do not generate a newline after each row." },
		{ "",   "id <arg>",      "Add and specify a unique ID column named <arg>."  },
		{ "",   "add-datestamps","Add columns for date created and date updated"  },
		{ "-a", "ascii",      "Remove any characters that aren't ASCII and reproduce"  },
		{ "",   "no-unsigned", "Remove any unsigned character sequences."  },
		{ "-T", "table <arg>",   "Use the specified table when connecting to a database for source data"  },
		{ "-Q", "query <arg>",   "Use this query to filter data from datasource"  },
		{ "-y", "stats",     "Dump stats at the end of an operation"  },
#if 0
		{ "-D", "datasource <arg>",   "Use the specified connection string to connect to a database for source data"  },
		{ "-L", "limit <arg>",   "Limit the result count to <arg>"  },
		{ "",   "buffer-size <arg>",  "Process only this many rows at a time when reading source data"  },
#endif
		{ "-X", "dumpdsn",       "Dump the DSN only. (DEBUG)" },
		{ "-h", "help",        "Show help." },
	};

	for ( int i = 0; i < sizeof(msgs) / sizeof(struct help); i++ ) {
		struct help h = msgs[i];
		fprintf( stderr, fmt, h.sarg, strlen( h.sarg ) ? "," : " ", h.larg, h.desc );
	}
	return 0;
}



int main ( int argc, char *argv[] ) {
	SHOW_COMPILE_DATE();

	// Define an input source, and an output source
	char err[ ERRLEN ];
	char *query = NULL;
	dsn_t input, output;
	char schema_fmt[ MAX_STMT_SIZE ];
	struct timespec stimer, etimer;

	// Initialize all the things
	memset( schema_fmt, 0, MAX_STMT_SIZE );
	memset( &stimer, 0, sizeof( struct timespec ) );
	memset( &etimer, 0, sizeof( struct timespec ) );
	memset( &input, 0, sizeof( dsn_t ) );
	memset( &output, 0, sizeof( dsn_t ) );
	memset( err, 0, sizeof( err ) );

	// This should already bee set to NULL, but...
	input.connstr = NULL;
	output.connstr = NULL;

	// Evaluate help or missing options
	if ( argc < 2 || !strcmp( *argv, "-h" ) || !strcmp( *argv, "--help" ) ) {
		return help();
	}

	for ( ++argv; *argv; ) {
		if ( !strcmp( *argv, "--no-unsigned" ) )
			no_unsigned = 1;	
		else if ( !strcmp( *argv, "--add-datestamps" ) )
			want_datestamps = 1;	
		else if ( !strcmp( *argv, "-t" ) || !strcmp( *argv, "--typesafe" ) )
			typesafe = 1;
		else if ( !strcmp( *argv, "-n" ) || !strcmp( *argv, "--no-newline" ) )
			newline = 0;
		else if ( !strcmp( *argv, "-j" ) || !strcmp( *argv, "--json" ) )
			stream_fmt = STREAM_JSON, convert = 1;
		else if ( !strcmp( *argv, "-x" ) || !strcmp( *argv, "--xml" ) )
			stream_fmt = STREAM_XML, convert = 1;
		else if ( !strcmp( *argv, "-q" ) || !strcmp( *argv, "--sql" ) )
			stream_fmt = STREAM_SQL, convert = 1;
		else if ( !strcmp( *argv, "--camel-case" ) )
			case_type = CASE_CAMEL;	
		else if ( !strcmp( *argv, "-S" ) || !strcmp( *argv, "--schema" ) )
			schema = 1;
		else if ( !strcmp( *argv, "-H" ) || !strcmp( *argv, "--headers" ) )
			headers_only = 1;	
		else if ( !strcmp( *argv, "-c" ) || !strcmp( *argv, "--convert" ) )
			convert = 1;	
		else if ( !strcmp( *argv, "-C" ) || !strcmp( *argv, "--coerce" ) ) {
			if ( !dupval( *( ++argv ), &coercion ) ) {
				return PERR( "%s\n", "No argument specified for --coerce." );
			}
		}
		else if ( !strcmp( *argv, "--id" ) ) {
			want_id = 1;	
			if ( !dupval( *( ++argv ), &id_name ) ) {
				return PERR( "%s\n", "No argument specified for --id." );
			}
		}
#if 0
		else if ( !strcmp( *argv, "-k" ) || !strcmp( *argv, "--schema" ) ) {
			schema = 1;
			if ( !dupval( *( ++argv ), &input.connstr ) ) {
				return PERR( "%s\n", "No argument specified for --schema." );
			}
		}
		else if ( !strcmp( *argv, "-r" ) || !strcmp( *argv, "--root" ) ) {
			if ( !dupval( *( ++argv ), &root ) ) {
				return PERR( "%s\n", "No argument specified for --root / --name." );
			}
		}
		else if ( !strcmp( *argv, "--name" ) ) {
			if ( !dupval( *( ++argv ), &root ) ) {
				return PERR( "%s\n", "No argument specified for --name / --root." );
			}
		}
#endif
		else if ( !strcmp( *argv, "--for" ) ) {
			if ( output.connstr ) {
				// Can't peicfy both output and --for
				return PERR( "%s\n", "Can't specify both --for & --output" );
			}

			if ( *( ++argv ) == NULL ) {
				PERR( "%s\n", "No argument specified for flag --for." );
				return 1;
			}

			if ( !strcasecmp( *argv, "sqlite3" ) ) {
				dbengine = SQL_SQLITE3;
				output.connstr = strdup( "sqlite3://" );
			}
		#ifdef BPGSQL_H
			else if ( !strcasecmp( *argv, "postgres" ) ) {
				dbengine = SQL_POSTGRES;
				output.connstr = strdup( "postgres://" );
			}
		#endif
		#ifdef BMYSQL_H
			else if ( !strcasecmp( *argv, "mysql" ) ) {
				dbengine = SQL_MYSQL;
				output.connstr = strdup( "mysql://" );
			}
		#endif
		#if 0
			else if ( !strcasecmp( *argv, "mssql" ) )
				dbengine = SQL_MSSQLSRV;
			else if ( !strcasecmp( *argv, "oracle" ) ) {
				dbengine = SQL_ORACLE;
				return PERR( "Oracle SQL dialect is currently unsupported, sorry...\n" );
			}
		#endif
			else {
				return PERR( "Argument specified '%s' for flag --for is an invalid option.", *argv );
			}

		}
		else if ( !strcmp( *argv, "-f" ) || !strcmp( *argv, "--format" ) ) {
			convert = 1;
			if ( !dupval( *( ++argv ), &streamtype ) )
				return PERR( "%s\n", "No argument specified for --format." );
			else if ( ( stream_fmt = check_for_valid_stream( streamtype ) ) == -1 ) {
				return PERR( "Invalid format '%s' requested.\n", streamtype );
			}	
		}
		else if ( !strcmp( *argv, "--class" ) ) {
			classdata = 1;
			serialize = 1;
			case_type = CASE_CAMEL;
#if 0
			if ( !dupval( *( ++argv ), &input.connstr ) ) {
				return PERR( "%s\n", "No argument specified for --class." );
			}
#endif
		}
		else if ( !strcmp( *argv, "--struct" ) ) {
			structdata = 1;
			serialize = 1;
#if 0
			if ( !dupval( *( ++argv ), &input.connstr ) ) {
				return PERR( "%s\n", "No argument specified for --struct." );
			}
#endif
		}
		else if ( !strcmp( *argv, "-a" ) || !strcmp( *argv, "--ascii" ) ) {
			ascii = 1;	
#if 0
			if ( !dupval( *( ++argv ), &input.connstr ) ) {
				return PERR( "%s\n", "No argument specified for --id." );
			}
#endif
		}
		else if ( !strcmp( *argv, "-p" ) || !strcmp( *argv, "--prefix" ) ) {
			if ( !dupval( *(++argv), &prefix ) )
			return PERR( "%s\n", "No argument specified for --prefix." );
		}
		else if ( !strcmp( *argv, "-x" ) || !strcmp( *argv, "--suffix" ) ) {
			if ( !dupval( *(++argv), &suffix ) )
			return PERR( "%s\n", "No argument specified for --suffix." );
		}
		else if ( !strcmp( *argv, "-d" ) || !strcmp( *argv, "--delimiter" ) ) {
			char *del = *(++argv);
			if ( !del ) {
				return PERR( "%s\n", "No argument specified for --delimiter." );
			}
			else if ( *del == 9 || ( strlen( del ) == 2 && !strcmp( del, "\\t" ) ) )  {
				DELIM = "\t";
			}
			else if ( !dupval( del, &DELIM ) ) {
				return PERR( "%s\n", "No argument specified for --delimiter." );
			}
		}
		else if ( !strcmp( *argv, "-u" ) || !strcmp( *argv, "--output-delimiter" ) ) {
			//Output delimters can be just about anything, 
			//so we don't worry about '-' or '--' in the statement
			if ( ! *( ++argv ) )
				return PERR( "%s\n", "No argument specified for --output-delimiter." );
			else {
				od = strdup( *argv );
				char *a = memchr( od, ',', strlen( od ) );
				if ( !a ) 
					ld = od, rd = od; 	
				else {
					*a = '\0';
					ld = od;	
					rd = ++a;
				}
			}	
		}
#if 0
		else if ( !strcmp( *argv, "-D" ) || !strcmp( *argv, "--datasource" ) ) {
			if ( !dupval( *(++argv), &input.connstr ) )
			return PERR( "%s\n", "No argument specified for --datasource." );
		}
#endif
		else if ( !strcmp( *argv, "-T" ) || !strcmp( *argv, "--table" ) ) {
			if ( !dupval( *(++argv), &table ) )
			return PERR( "%s\n", "No argument specified for --table." );
		}
		else if ( !strcmp( *argv, "-Q" ) || !strcmp( *argv, "--query" ) ) {
			if ( !dupval( *(++argv), &query ) )
			return PERR( "%s\n", "No argument specified for --query." );
		}
		else if ( !strcmp( *argv, "-i" ) || !strcmp( *argv, "--input" ) ) {
		#if 0
			char *arg = *( ++argv );
			if ( !arg || !dupval( arg, &input.connstr ) ) {
				return PERR( "%s\n", "No argument specified for --input." );
			}
		#endif
			if ( !dupval( *( ++argv ), &input.connstr ) ) {
				return PERR( "%s\n", "No argument specified for --input." );
			}
		}
		else if ( !strcmp( *argv, "-o" ) || !strcmp( *argv, "--output" ) ) {
			char *arg = *( ++argv );
			if ( !arg || !dupval( arg, &output.connstr ) ) {
				return PERR( "%s\n", "No argument specified for --output." );
			}
		}
		else if ( !strcmp( *argv, "-X" ) || !strcmp( *argv, "--dumpdsn" ) ) {
			dump_parsed_dsn = 1;
		}
		else if ( !strcmp( *argv, "-y" ) || !strcmp( *argv, "--stats" ) ) {
			show_stats = 1;
		}
		else {
			fprintf( stderr, "Got unknown argument: %s\n", *argv );
			return help();
		}
		argv++;
	}

	// Dump the DSN(s)
	if ( dump_parsed_dsn ) {
		if ( !parse_dsn_info( &input, err, sizeof( err ) ) ) {
			PERR( "Input file was invalid: %s\n", err );
			return 1;
		}

		print_dsn( &input );
#if 0
		if ( output.connstr ) {
			if ( !parse_dsn_info( &output, err, sizeof( err ) ) ) {
				PERR( "Output file was invalid: %s\n", err );
				return 1;
			}

			print_dsn( &output );
		}
#endif
		return 0;
	}

	// Start the timer here
	if ( show_stats )
		clock_gettime( CLOCK_REALTIME, &stimer );

	// If the user didn't actually specify an action, we should close and tell them that 
	// what they did is useless.
	if ( !convert && !schema && !headers_only ) {
		return PERR( "No action specified, exiting.\n" );
	}

	// The dsn is always going to be the input
	if ( !parse_dsn_info( &input, err, sizeof( err  ) ) ) {
		return PERR( "Input source is invalid: %s\n", err );
	}

	// Open the DSN first (regardless of type)	
	if ( !open_dsn( &input, query, err, sizeof( err ) ) ) {
		PERR( "DSN open failed: %s.\n", err );
		return 0;
	}

	// Create the header_t here
	if ( !headers_from_dsn( &input, err, sizeof( err ) ) ) {
		PERR( "Header creation failed: %s.\n", err );
		return 0;
	}

	// Processor stuff
	if ( headers_only ) {
		for ( header_t **s = input.headers; s && *s; s++ ) {
			printf( "%s\n", (*s)->label );
		}
	}

	else if ( schema ) {
		char *schbuf = NULL;	
#if 1
		// If a connection string was specified, try parsing for info
		if ( output.connstr ) {
			if ( !parse_dsn_info( &output, err, sizeof( err  ) ) ) {
				PERR( "Output sink is invalid: %s\n", err );
				return 0;
			}

			// Set the table if the user specified one
			if ( table ) {
				if ( strlen( table ) >= sizeof( output.tablename ) ) {
					PERR( "Table name '%s' is too long for buffer.\n", table );
					return 0;
				}
				snprintf( output.tablename, sizeof( output.tablename ), "%s", table );
			}
		}

		// If a table only was specified, use it as the output source
		else if ( table ) {
			if ( strlen( table ) < 1 ) {
				PERR( "Table name is invalid or unspecified for --schema command.\n" );
				return 0;
			}

			if ( strlen( table ) >= sizeof( output.tablename ) ) {
				PERR( "Table name '%s' is too long for buffer.\n", table );
				return 0;
			}

			snprintf( output.tablename, sizeof( output.tablename ), "%s", table ); 
		}

		// Finally, if nothing was specified, assume the input source has the table name
		else {
			if ( !strlen( input.tablename ) ) {
				PERR( "Table name is invalid or unspecified for --schema command.\n" );
				return 0;
			}
	
			snprintf( output.tablename, sizeof( output.tablename ), "%s", input.tablename ); 
		}

		// Set for SQLite by default
		if ( !output.type ) {
			output.type = DB_SQLITE;
		}

fprintf( stdout, "OUTPUT CONNSTR: %s\n", output.connstr );
//print_dsn( &input );print_dsn( &output );

		// Figure out any relevant mappings
		if ( !rels_between_dsn( &input, &output, err, sizeof( err ) ) ) {
			PERR( "Rel build failed: %s\n", err );
			return 0;
		}

		// Get the types of each thing in the column
		if ( !types_from_dsn( &input, &output, coercion, err, sizeof( err ) ) ) {
			PERR( "typeget failed: %s\n", err );
			return 0;
		}

#if 1
for ( const typemap_t *t = output.typemap; t->ntype != TYPEMAP_TERM; t++ ) 
printf( "typename: %s (%s)\n", t->typename, t->libtypename );
#endif

#endif
		//	
		if ( !schema_from_dsn( &input, &output, schema_fmt, MAX_STMT_SIZE, err, sizeof( err ) ) ) {
			free( output.connstr );
			destroy_dsn_headers( &input );
			close_dsn( &input );
			PERR( "Schema build failed: %s\n", err );
			return 0;
		}
		// Dump said schema 
		fprintf( stdout, "%s", schema_fmt );

		// Destroy the output string
		free( output.connstr );

		schbuf = NULL;
exit(0);
	}

#if 0
	else if ( classdata || structdata ) {
		DPRINTF( "classdata || structdata only was called...\n" );
		//if ( !struct_f( FFILE, DELIM ) ) return 1;
	}
#endif

	else if ( convert ) {
		DPRINTF( "convert was called...\n" );
		if ( !prepare_dsn( &input, err, sizeof( err ) ) ) {
			destroy_dsn_headers( &input );
			close_dsn( &input );
			return PERR( "Failed to prepare DSN: %s.\n", err );
		}

		// Use this to figure out whether or not the output DSN is prepared the right way
		// ...

		// Then open a new DSN for the output (there must be an --output flag)
		if ( !output.connstr ) {
			output.output = stdout; 
		}
		else {
			// Output
			//memset( &output, 0, sizeof( dsn_t ) );

			// Parse the output source
			if ( !parse_dsn_info( &output, err, sizeof( err ) ) ) {
				return PERR( "Output DSN is invalid: %s\n", err );
			}

		#if 0
			// Test dsn? (to see if the source already exists)
			if ( !test_dsn( &output, err, sizeof( err ) ) ) {

			}
		#endif

			// Create an output dsn (unless it already exists)
			if ( !create_dsn( &output, &input, err, sizeof( err ) ) ) {
				fprintf( stderr, "Creating output DSN failed: %s\n", err );
				return 1;
			}

			// Open the output dsn
			if ( !open_dsn( &output, query, err, sizeof( err ) ) ) {
				PERR( "DSN open failed: %s\n", err );
				return 1;
			}

			// If this is a database, choose the stream type
			if ( 0 ) ;
		#ifdef BSQLITE_H
			else if ( output.type == DB_SQLITE )
				stream_fmt = STREAM_SQLITE;
		#endif
		#ifdef BMYSQL_H
			else if ( output.type == DB_MYSQL )
				stream_fmt = STREAM_MYSQL;
		#endif
		#ifdef BPGSQL_H
			else if ( output.type == DB_POSTGRESQL )
				stream_fmt = STREAM_PGSQL;
		#endif
			else {
			}
		} /* end else */


		// Control the streaming / buffering from here
		for ( int i = 0; i < 1; i++ ) {

			// Stream to records
			if ( !records_from_dsn( &input, 0, 0, err, sizeof( err ) ) ) {
				destroy_dsn_headers( &input );
				close_dsn( &input );
				return PERR( "Failed to generate records from data source: %s.\n", err );
			}

			// Do the transport (buffering can be added pretty soon)
			if ( !transform_from_dsn( &input, &output, stream_fmt, 0, 0, err, sizeof(err) ) ) {
				fprintf( stderr, "Failed to transform records from data source: %s\n", err );
				destroy_dsn_headers( &input );
				close_dsn( &input );
				return 1;
			}

		} // end for
	
		// close our open data source		
		close_dsn( &output ); 
	}

	// Show the stats if we did that
	if ( show_stats ) {
		clock_gettime( CLOCK_REALTIME, &etimer );
		fprintf( stdout, "%d record(s) copied...\n", input.rlen ); 
		fprintf( stdout, "Time elapsed: %ld.%ld\n", 
			etimer.tv_sec - stimer.tv_sec, 
			etimer.tv_nsec - stimer.tv_nsec 
		);
	}

	// Destroy the rows
	destroy_dsn_rows( &input );
	
	// Destroy the headers
	destroy_dsn_headers( &input );

	// Close the DSN
	close_dsn( &input ); 
	return 0;
}
