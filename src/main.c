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
#include <sys/mman.h>
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
 #include <sqlite3.h>
#endif

/* Terminating character in typemaps */
#define TYPEMAP_TERM INT_MIN

/* Print an error code and return 1 (false) on the command line */
#define ERRPRINTF( C, ... ) \
	fprintf( stderr, "briggs: " ) && (fprintf( stderr, __VA_ARGS__ ) ? C : C )

/* Eventually, these will be real return codes (just not now) */
#define ERRCODE 1

/* Show the compilation date */
#define SHOW_COMPILE_DATE() \
	fprintf( stderr, "briggs v" VERSION " compiled: " __DATE__ ", " __TIME__ "\n" )

/* Compare against a string literal */
#define STRLCMP(H,S) \
	( strlen( H ) >= sizeof( S ) - 1 ) && !memcmp( H, S, sizeof( S ) - 1 )

/* Compare against long & short options */
#define OPTCMP(H,S,L) \
	( strlen( H ) >= sizeof( S ) - 1 ) && !memcmp( H, S, sizeof( S ) - 1 )

/* Compare against a string literal against somewhere in memory */
#define MEMLCMP(H,S,L) \
	( L >= sizeof( S ) - 1 ) && !memcmp( H, S, sizeof( S ) - 1 )

/* Write to a file descriptor */
#define FDPRINTF(fd, X) \
	write( fd, X, strlen( X ) )

/* Write a block of length to a file descriptor */
#define FDNPRINTF(fd, X, L) \
	write( fd, X, L )

/* Malloc and memset (or just use calloc, b/c it's probably faster :) ) */
#define CALLOC_NEW_FAILS(O,S) \
	!( O = malloc( sizeof( S ) ) ) || !memset( O, 0, sizeof( S ) )

/* Try to include rudimentary newline support */
#ifdef WIN32
 #define NEWLINE "\r\n"
#else
 #define NEWLINE "\n"
#endif

/* Debugging rules */
#ifndef DEBUG_H
 #define DPRINTF( ... ) 0
 #define N(A) ""
 #define print_dsn(A) 0
 #define print_headers(A) 0
 #define print_stream_type(A) 0
#else
	/* Optionally print arguments */
 #define DPRINTF( ... ) fprintf( stderr, __VA_ARGS__ )
	/* Print the literal name of a type */
 #define N(A) #A
#endif


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
 * format_t
 *
 * Control formatting when printing data structures
 *
 */
typedef struct format_t {
	int pretty;
	int newline;
	int spaced;
	int ccase;
	char *prefix;
	char *suffix;
	char *leftdelim;
	char *rightdelim;
} format_t;

static format_t FF = { 1, 1, 0, 0, NULL, NULL, "'", "'" };


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
	CASE_NONE = 0,
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
 * file_t
 *
 * In lieu of simply using fread/fwrite, this should allow me to read VERY large files easily
 *
 */
typedef struct file_t {
	int fd;  // a file handle
	void *map;  // makes reading and writing much easier
	const void *start;  // free this at the end
	unsigned long size; // the file's size
	unsigned int offset;
// int limit;
} file_t;



typedef struct mysql_t {
	MYSQL *conn;
	MYSQL_STMT *stmt;
	MYSQL_BIND *bindargs;
	MYSQL_RES *res;
} mysql_t;



typedef struct pgsql_t {
	PGconn *conn;
	PGresult *res;
} pgsql_t;


#if 0
typedef struct sqlite3_t {
	sqlite3 *conn;
	sqlite3_stmt *stmt;
} sqlite3_t;
#endif


/**
 * header_t
 *
 * General format for headers or column names coming from a datasource
 *
 */
typedef struct header_t {
	/* Name of the column */
	char label[ 128 ];

	/* Whether or not this column should be treated as a primary key */
	int primary;

	/* Options that may have come from a database engine */
	int options;

	/* The native type used to save this value in the original datasource */
	//int ntype;

	/* The matched type used to convert this value into a format suitable for another datasource */
	int mtype;

	/* A sensible base type */
	type_t type;

	/* Native matched type */
	typemap_t *ctype;

	/* A coerced type (preferably a name) */
	typemap_t *ntype;
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
	void *conn;
	//void *res;
	header_t **headers;
	row_t **rows;
	int hlen;
	int rlen;
	int clen;
	//char **defs;
	//FILE *output;
	const typemap_t *typemap;
	const typemap_t *convmap;
	int offset;
	int size;
	stream_t stream;
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


typedef struct coerce_t {
	const char name[ 128 ];
	const char typename[ 64 ];
} coerce_t;


typedef struct config_t {

	stream_t wstream;	// wstream - A chosen stream
	char wdeclaration;	// wdeclaration - Dump a class or struct declaration only 
	char wschema;	// wschema - Dump the schema only
	char wheaders;	// wheaders - Dump the headers only
	char wconvert;	// wconvert - Convert the source into some other format
	char wclass;	// wclass - Convert to a class
	char wstruct;	// wstruct - Convert to a struct
	char wtypesafe;
	char wnewline;
	char wdatestamps;
	char wnounsigned;
	char wspcase;
	char wid;   // Use a unique ID when reading from a datasource
	char wstats;   // Use the stats
	
	char *wcutcols;  // The user wants to cut columns
	char *widname;  // The unique ID column name
	char *wcoerce;  // The user wants to override certain types
	char *wquery;	// A custom query from the user
	char *wprefix;
	char *wsuffix; 
	char *wtable; 

} config_t;




// This is for default types.  A map IS a lot of overhead...
const typemap_t default_map[] = {
	{ T_NULL, N(T_NULL), NULL, T_NULL, 0 },
	{ T_STRING, N(T_STRING), "TEXT", T_STRING, 1 },
	/* NOTE: This will force STRING to take precedence over CHAR.  DO NOT TOUCH THIS */
	{ T_CHAR, N(T_CHAR), "TEXT", T_CHAR, 1 },
	{ T_INTEGER, N(T_INTEGER), "INTEGER", T_INTEGER, 1 },
	{ T_DOUBLE, N(T_DOUBLE), "REAL", T_DOUBLE, 1 },
	{ T_BINARY, N(T_BINARY), "BLOB", T_BINARY, 1 },
	{ T_BOOLEAN, N(T_BOOLEAN), "BOOLEAN", T_BOOLEAN, 1 },

	// Supporting base dates is EXTREMELY difficult... be careful supporting it
	//{ T_BOOLEAN, N(T_BOOLEAN), "BOOLEAN", T_BOOLEAN },
	{ TYPEMAP_TERM },
};


const typemap_t sqlite3_map[] = {
	{ T_NULL, N(T_NULL), NULL, T_NULL, 0 },
	{ T_STRING, N(T_STRING), "TEXT", T_STRING, 1 },
	/* NOTE: This will force STRING to take precedence over CHAR.  DO NOT TOUCH THIS */
	{ T_CHAR, N(T_CHAR), "TEXT", T_CHAR, 1 },
	{ T_INTEGER, N(T_INTEGER), "INTEGER", T_INTEGER, 1 },
	{ T_DOUBLE, N(T_DOUBLE), "REAL", T_DOUBLE, 1 },
	{ T_BINARY, N(T_BINARY), "BLOB", T_BINARY, 1 },
	{ T_INTEGER, N(T_INTEGER), "INTEGER", T_BOOLEAN, 1 },
	{ TYPEMAP_TERM },
};


static const char * dbtypes[] = {
	[DB_NONE] = "none",
	[DB_FILE] = "file",
	[DB_SQLITE] = "SQLite3",
	[DB_MYSQL] = "MySQL",
	[DB_POSTGRESQL] = "Postgres",
};


static const char * get_conn_type( dbtype_t t ) {
	return ( t >= 0 && t < sizeof( dbtypes ) / sizeof( char * ) ) ? dbtypes[ t ] : NULL; 
}


static const char * get_btypename ( type_t t ) {
	return ( t >= 0 && (int)t < sizeof( itypes ) / sizeof( char * ) ) ? itypes[ t ] : NULL;
}


typemap_t * get_typemap_by_ntype ( const typemap_t *types, int type ) {
	for ( const typemap_t *t = types; t->ntype != TYPEMAP_TERM; t++ ) {
		if ( t->ntype == type ) return (typemap_t *)t;
	}
	return NULL;
}


typemap_t * get_typemap_by_nname ( const typemap_t *types, const char *name ) {
	for ( const typemap_t *t = types; t->ntype != TYPEMAP_TERM; t++ ) {
		if ( !strcasecmp( name, t->typename ) ) {
			return (typemap_t *)t;
		}
	}
	return NULL;
}


typemap_t * get_typemap_by_etype ( const typemap_t *types, int type ) {
	for ( const typemap_t *t = types; t->ntype != TYPEMAP_TERM; t++ ) {
		if ( t->ntype == type ) return (typemap_t *)t;
	}
	return NULL;
}


// Use this to look for matches by basetype
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

static const typemap_t pgsql_map[] = {
#if 0
	{ TYPENAME(PG_BOOLOID), "bool", T_BOOLEAN },
#endif
	//{ PG_INT2OID, N(PG_INT2OID), "smallint", T_INTEGER, 0 },
	{ PG_INT2OID, N(PG_INT2OID), "smallint", T_INTEGER, 0 },
	{ PG_INT4OID, N(PG_INT4OID), "int", T_INTEGER, 1, },
	{ PG_INT4OID, N(PG_INT4OID), "integer", T_INTEGER, 0, },
	{ PG_INT8OID, N(PG_INT8OID), "bigint", T_INTEGER, 0 },
	{ PG_NUMERICOID, N(PG_NUMERICOID), "numeric", T_DOUBLE, 0 },
	{ PG_FLOAT4OID, N(PG_FLOAT4OID), "real", T_DOUBLE, 1 }, /* or float4 */
	{ PG_FLOAT8OID, N(PG_FLOAT8OID), "double precision", T_DOUBLE, 0 }, /* or float8 */
	{ PG_TEXTOID, N(PG_TEXTOID), "text", T_STRING, 1 },
	{ PG_BPCHAROID, N(PG_BPCHAROID), "varchar", T_STRING, 0 },
	{ PG_VARCHAROID, N(PG_VARCHAROID), "varchar", T_CHAR, 1 },
	{ PG_BYTEAOID, N(PG_BYTEAOID), "bytea", T_BINARY, 1 },
	{ PG_BYTEAOID, N(PG_BYTEAOID), "blob", T_BINARY, 0 },
	{ PG_BOOLOID, N(PG_BOOLOID), "boolean", T_BOOLEAN, 1 },
	{ TYPEMAP_TERM },
#if 0
	[PG_DATEOID] = "PG_DATEOID",
	[PG_TIMEOID] = "PG_TIMEOID",
	[PG_TIMESTAMPOID] = "PG_TIMESTAMPOID",
	[PG_TIMESTAMPZOID] = "PG_TIMESTAMPZOID",
	[PG_TIMETZOID] = "PG_TIMETZOID",
	[PG_DATETIMEOID] = "PG_DATETIMEOID",
#endif
#if 0
	[PG_OIDOID] = "PG_OIDOID",
	[PG_BITOID] = "PG_BITOID",
	[PG_VARBITOID] = "PG_VARBITOID",
#endif
};
#endif



#ifdef BMYSQL_H
static const typemap_t mysql_map[] = {
	{ MYSQL_TYPE_TINY, N(MYSQL_TYPE_TINY), "TINYINT", T_INTEGER },
	{ MYSQL_TYPE_SHORT, N(MYSQL_TYPE_SHORT), "SMALLINT", T_INTEGER },
	{ MYSQL_TYPE_LONG, N(MYSQL_TYPE_LONG), "INT", T_INTEGER, 1 },
	{ MYSQL_TYPE_LONG, N(MYSQL_TYPE_LONG), "INTEGER", T_INTEGER, 0 },
	{ MYSQL_TYPE_INT24, N(MYSQL_TYPE_INT24), "MEDIUMINT", T_INTEGER },
	{ MYSQL_TYPE_LONGLONG, N(MYSQL_TYPE_LONGLONG), "BIGINT", T_INTEGER },
	{ MYSQL_TYPE_DECIMAL, N(MYSQL_TYPE_DECIMAL), "DECIMAL", T_DOUBLE, 1 }, /* or NUMERIC */
	{ MYSQL_TYPE_NEWDECIMAL, N(MYSQL_TYPE_NEWDECIMAL), "NUMERIC", T_DOUBLE }, /* or NUMERIC */
	{ MYSQL_TYPE_FLOAT, N(MYSQL_TYPE_FLOAT), "FLOAT", T_DOUBLE },
	{ MYSQL_TYPE_DOUBLE, N(MYSQL_TYPE_DOUBLE), "DOUBLE", T_DOUBLE },
	{ MYSQL_TYPE_STRING, N(MYSQL_TYPE_STRING), "CHAR", T_CHAR, 1 },
	{ MYSQL_TYPE_VAR_STRING, N(MYSQL_TYPE_VAR_STRING), "VARCHAR", T_STRING, 1 },
	{ MYSQL_TYPE_BLOB, N(MYSQL_TYPE_BLOB), "BLOB", T_BINARY, 1 },
	/* This just maps to TINYINT behind the scenes */ 
	{ MYSQL_TYPE_TINY, N(MYSQL_TYPE_TINY), "BOOL", T_BOOLEAN, 1 },
#if 0
	{ MYSQL_TYPE_TIMESTAMP, N(MYSQL_TYPE_TIMESTAMP), "TIMESTAMP", T_XXX },
	{ MYSQL_TYPE_DATE, N(MYSQL_TYPE_DATE), "DATE", T_XXX },
	{ MYSQL_TYPE_TIME, N(MYSQL_TYPE_TIME), "TIME", T_XXX },
	{ MYSQL_TYPE_DATETIME, N(MYSQL_TYPE_DATETIME), "DATETIME", T_XXX },
	{ MYSQL_TYPE_YEAR, N(MYSQL_TYPE_YEAR), "YEAR", T_XXX },
#endif
	{ TYPEMAP_TERM },
};

#endif


// Forward declarations for different output formats
void p_default( int, column_t * );
void p_xml( int, column_t * );
void p_comma( int, column_t * );
void p_cstruct( int, column_t * );
void p_carray( int, column_t * );
void p_sql( int, column_t * );
void p_json( int, column_t * );


// Global variables to ease testing
char delset[ 8 ] = { 13 /*'\r'*/, 10 /*'\n'*/, 0, 0, 0, 0, 0, 0 };
const char *STDIN = "/dev/stdin";
const char *STDOUT = "/dev/stdout";
char *no_header = "nothing";
char *output_file = NULL;
char *FFILE = NULL;
char *OUTPUT = NULL;
char *INPUT = NULL;
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
// TODO: Strictly, this isn't acting on anything that would be dependent on a char *definition, butthis should really be rewritten to return an unsigned char *
static char * copy( char *src, int size, int *ns, case_t t ) {
	char *a = NULL;
	char *b = NULL;
	int nsize = 0;

	if ( !( a = malloc( size + 1 ) ) || !memset( a, 0, size + 1 ) ) {
		return NULL;
	}

	b = (char *)trim( (unsigned char *)src, " ", size, &nsize );
	memcpy( a, b, nsize );

	if ( t == CASE_SNAKE )
		snakecase( &a );
	else if ( t == CASE_CAMEL ) {
		camelcase( &a );
	}

	*ns = nsize;
	return a;
}




// Check the type of a value in a field
static type_t get_type( unsigned char *v, type_t deftype, unsigned int len ) {
	type_t t = T_INTEGER;

	// If the value is blank, then we need some help...
	// Use deftype in this case...
	//if ( strlen( v ) == 0 ) {
	if ( !len )
		return deftype;

	// If it's just one character, should check if it's a number or char
	if ( len == 1 ) {
		// TODO: Can do boolean evaluation here too
		if ( memchr( "TtFf", *v, 4 ) ) {
			t = T_BOOLEAN;
		}

		// TODO: Code should be 0-127
		else if ( !memchr( "0123456789", *v, 10 ) ) {
			t = T_CHAR;
		}
		return t;
	}

	// Boolean true (regardless of spelling)
	if ( len >= 4 && ( *v == 't' || *v == 'T' ) ) {
		unsigned char *vv = v;
		// We can do this the slowest possible way b/c it's a pain in the butt
		if ( *vv == 'r' || *vv == 'R' )
			vv++;
		if ( *vv == 'u' || *vv == 'U' )
			vv++;
		if ( *vv == 'e' || *vv == 'E' ) {
			t = T_BOOLEAN;
			return t;
		}
	}

	// Boolean false (regardless of spelling)
	if ( len >= 5 && ( *v == 'f' || *v == 'F' ) ) {
		unsigned char *vv = v;
		// We can do this the slowest possible way b/c it's a pain in the butt
		if ( *vv == 'a' || *vv == 'A' )
			vv++;
		if ( *vv == 'l' || *vv == 'L' )
			vv++;
		if ( *vv == 's' || *vv == 'S' )
			vv++;
		if ( *vv == 'e' || *vv == 'E' ) {
			t = T_BOOLEAN;
			return t;
		}
	}

	// If we start w/ '-', then this COULD mean that we're dealing with negative numbers
	if ( *v == '-' )
		v++, len--;

	// Check for either REAL, BINARY or STRING
	for ( unsigned char *vv = v; *vv && len; vv++, len-- ) {
		if ( *vv == '.' && ( t == T_INTEGER ) )
			t = T_DOUBLE;

		// Allow for horizontal tabs and spaces
		if ( *v != 9 && ( *vv < 32 || *vv > 126 ) ) {
			t = T_BINARY;
			return t;
		}

		if ( !memchr( "0123456789.", *vv, 11 ) ) {
			t = T_STRING;
			return t;
		}
	}

	return t;
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
static unsigned char * extract_row( void *buf, int buflen, unsigned int *newlen ) {
	// Extract just the first row of real values
	unsigned char *str = NULL;
	unsigned char *start = NULL;
	int slen = 0;

	// Intended for use with
	if ( !( start = memchr( buf, '\n', buflen ) ) ) {
		return NULL;
	}

	// Increment until I find the next row (assuming that the file is line based)
	for ( unsigned char *s = ++start; ( *s != '\n' && *s != '\r' ); s++, slen++ );

	// Allocate for the portion we want
	if ( !( str = malloc( ++slen + 1 ) ) || !memset( str, 0, slen + 1 ) ) {
		return NULL;
	}

	memcpy( str, start, slen );
	*newlen = slen;
	return str;
}



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


/*
 * Copy to a number safely.
 *
 */
int safecpynumeric( char *numtext ) {
	for ( char *p = numtext; *p; p++ ) {
		if ( !memchr( "0123456789", *p, 10 ) ) return -1;
	}

	return atoi( numtext );
}


/*
 * Copy to a number safely from unsigned character block.
 *
 */
int usafecpynumeric( unsigned char *numtext, long l ) {
	long len = l;
	char *jp = NULL;
	int num = 0;

	// Check for the stupid thing
	for ( unsigned char *p = numtext; len && *p; p++, len-- )
		if ( !memchr( "0123456789", *p, 10 ) ) return -1;

	//Copy into a buffer to use atoi
	if ( !( jp = malloc( l + 1 ) ) || !memset( jp, 0, l ) ) {
		return -1;
	}

	memcpy( jp, numtext, len );	
	num = atoi( jp );
	free( jp );
	return num;
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
	else if ( STRLCMP( dsn, "mysql://" ) ) {
		conn->type = DB_MYSQL;
		conn->port = 3306;
		conn->typemap = mysql_map;
		dsn += 8, dsnlen -= 8;
	}
#endif
#ifdef BPGSQL_H
	else if ( ( STRLCMP( dsn, "postgresql://" ) ) || ( STRLCMP( dsn, "postgres://" ) ) ) {
		const int len = STRLCMP( dsn, "postgresql://" ) ? 13 : 11;
		conn->type = DB_POSTGRESQL;
		conn->port = 5432;
		conn->typemap = pgsql_map;
		dsn += len, dsnlen -= len;
	}
#endif
	else if ( STRLCMP( dsn, "sqlite3://" ) ) {
		conn->type = DB_SQLITE;
		conn->port = 0;
		conn->typemap = default_map;
		dsn += 10, dsnlen -= 10;
	}
	else {
		if ( strlen( dsn ) == 1 && *dsn == '-' )
			conn->connstr = "/dev/stdin", dsnlen -= 1, dsn += 1;
		else if ( STRLCMP( dsn, "file://" ) ) {
			// Use memmove to get rid of the file:// prefix
			conn->connstr += 7, dsn += 7, dsnlen -= 7;
		}

		// TODO: Check valid extensions
		conn->type = DB_FILE;
		conn->port = 0;
		conn->typemap = default_map;
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




#ifdef DEBUG_H

// Dump the DSN
void print_dsn ( dsn_t *conninfo ) {
	fprintf( stderr, "dsn->connstr:    '%s'\n", conninfo->connstr );
	fprintf( stderr, "dsn->type:       %s\n",   dbtypes[ conninfo->type ] );
	fprintf( stderr, "dsn->username:   '%s'\n", conninfo->username );
	fprintf( stderr, "dsn->password:   '%s'\n", conninfo->password );
	fprintf( stderr, "dsn->hostname:   '%s'\n", conninfo->hostname );
	fprintf( stderr, "dsn->dbname:     '%s'\n", conninfo->dbname );
	fprintf( stderr, "dsn->tablename:  '%s'\n", conninfo->tablename );
	//fprintf( stderr, "dsn->socketpath: '%s'\n", conninfo->socketpath );
	fprintf( stderr, "dsn->port:       %d\n", conninfo->port );
	fprintf( stderr, "dsn->conn:       %p\n", conninfo->conn );
	fprintf( stderr, "dsn->options:    '%s'\n", conninfo->connoptions );
	fprintf( stderr, "dsn->typemap:    '%p' ", (void *)conninfo->typemap );

	if ( conninfo->typemap == default_map )
		fprintf( stderr, "(default)" );
	else if ( conninfo->typemap == sqlite3_map )
		fprintf( stderr, "(SQLite)" );
	else if ( conninfo->typemap == mysql_map )
		fprintf( stderr, "(MySQL)" );
	else if ( conninfo->typemap == pgsql_map ) {
		fprintf( stderr, "(PostgreSQL)" );
	}
	fprintf( stderr, "\n" );

	//if headers have been initialized show me that
	fprintf( stderr, "dsn->headers:    %p\n", (void *)conninfo->headers );
	if ( conninfo->headers ) {
		for ( header_t **h = conninfo->headers; h && *h; h++ ) {
			fprintf( stderr, "  %-30s [%s, %s, %s]\n",
				(*h)->label,
				(*h)->ctype ? "?" : "(nil)",
				(*h)->ntype ? (*h)->ntype->typename : "(nil)",
				itypes[ (*h)->type ]
			);
		}
	}
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

	// Depends on output chosen
	int len = iconn->hlen;
	int written = 0;
	char *fmt = fm;
	const char stmtfmt[] = "CREATE TABLE IF NOT EXISTS %s (\n";

	// Schema creation won't work if no table name is specified.  Stop here if that's so.
	if ( !strlen( oconn->tablename ) ) {
		snprintf( err, errlen, "No table name was specified." );
		return 0;
	}

	// If there is no schema chosen, use SQLite as the default
	if ( oconn->type == DB_FILE && oconn->typemap == default_map ) {
fprintf( stderr, "[[[ !!! Changing to default engine... SQLite !!! ]]]\n" );
		oconn->typemap = sqlite3_map;
	}

	// Write the first line
	written = snprintf( fmt, fmtlen, stmtfmt, oconn->tablename );
	fmtlen -= written, fmt += written;

#if 0
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
#endif

	// SQL schema creation will
	// Generate a line for each row
	for ( header_t **s = iconn->headers; s && *s; s++, len-- ) {
		// Getting the type needs to be a bit smarter now
		typemap_t *type = NULL;

		if ( (*s)->ctype ) {
			type = (*s)->ctype;
		}
		else if ( !( type = get_typemap_by_btype( oconn->typemap, (*s)->type ) ) ) {
			const char fmt[] = "No available type found for column '%s' %s";
			snprintf( err, errlen, fmt, (*s)->label, itypes[ (*s)->type ] );
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
 * test_dsn( dsn_t * )
 * ===================
 *
 * Test a data source for existence.
 *
 */
int test_dsn (dsn_t *conn, char *err, int errlen) {
	if ( conn->type == DB_FILE ) {
		struct stat sb;
		memset( &sb, 0, sizeof( struct stat ) );

		// Anything other than file not found is a problem
		if ( access( conn->connstr, F_OK | R_OK ) == -1 ) {
			snprintf( err, errlen, "%s", strerror( errno ) );
			return 0;
		}

		if ( stat( conn->connstr, &sb ) == -1 ) {
			snprintf( err, errlen, "%s", strerror( errno ) );
			return 0;
		}

		return 1;
	}
	else if ( conn->type == DB_SQLITE ) {
		// Same here.  Access could technically be used with the database name or path specified
	}
	else if ( conn->type == DB_MYSQL ) {
		// You'll need to be able to connect and test for existence of db and table

	}
	else if ( conn->type == DB_POSTGRESQL ) {
		// You'll need to be able to connect and test for existence of db and/or table

	}
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
int create_dsn ( dsn_t *iconn, dsn_t *oconn, char *err, int errlen ) {
	// This might be a little harder, unless there is a way to either create a new connection
	char schema_fmt[ MAX_STMT_SIZE ];
	memset( schema_fmt, 0, MAX_STMT_SIZE );
#if 0
	const char create_database_stmt[] = "CREATE DATABASE IF NOT EXISTS %s";
#endif

	// Simply create the file.  Do not open it
	if ( oconn->type == DB_FILE ) {
		const int flags =	O_RDWR | O_CREAT | O_TRUNC;
		const mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
		if ( open( oconn->connstr, flags, perms ) == -1 ) {
			snprintf( err, errlen, "%s", strerror( errno ) );
			return 0;
		}
		return 1;
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


int wsize = 0;




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
		DPRINTF( "Attempting to open file at '%s'\n", conn->connstr );

		// An mmap() implementation for speedy reading and less refactoring
		int fd;
		struct stat sb;
		file_t *file = NULL;

		// Create a file structure
		if ( !( file = malloc( sizeof( file_t ) ) ) || !memset( file, 0, sizeof( file_t ) ) ) {
			snprintf( err, errlen, "malloc() %s\n", strerror( errno ) );
			return 0;
		}

		// Handle stdout (since there will be no size, and it doesn't make tons of sense to mmap() it)
		if ( STRLCMP( conn->connstr, "/dev/stdout" ) ) {
			file->fd = 1;
			file->start = file->map = NULL;
			conn->conn = file;
			return 1;
		}

		// Initialize the stat buffer and check for the file
		if ( memset( &sb, 0, sizeof( struct stat ) ) && stat( conn->connstr, &sb ) == -1 ) {
			snprintf( err, errlen, "stat() %s\n", strerror( errno ) );
			free( file );
			return 0;
		}

		// Open the file
		if ( ( file->fd = open( conn->connstr, O_RDONLY ) ) == -1 ) {
			snprintf( err, errlen, "open() %s\n", strerror( errno ) );
			free( file );
			return 0;
		}

		// Do stuff
		file->size = sb.st_size;
		file->map = mmap( 0, file->size, PROT_READ, MAP_PRIVATE, file->fd, 0 );
		if ( file->map == MAP_FAILED ) {
			snprintf( err, errlen, "mmap() %s\n", strerror( errno ) );
			free( file );
			return 0;
		}

		file->start = file->map;
		file->offset = 0;
		conn->conn = file;
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
		// Switch everything to use this format now
		mysql_t *db = NULL;

		if ( !( db = malloc( sizeof( mysql_t ) ) ) || !memset( db, 0, sizeof( mysql_t ) ) ) {
			const char fmt[] = "Memory constraint encountered initializing MySQL connection.";
			snprintf( err, errlen, fmt );
			return 0;
		}

		// Initialize the connection
		if ( !( db->conn = mysql_init( 0 ) ) ) {
			const char fmt[] = "Couldn't initialize MySQL connection: %s";
			snprintf( err, errlen, fmt, mysql_error( db->conn ) );
			mysql_close( db->conn );
			return 0;
		}

		// Open the connection
		MYSQL *t = mysql_real_connect(
			db->conn,
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
			snprintf( err, errlen, fmt, mysql_error( t ) );
			return 0;
		}

		// Execute whatever query
		if ( mysql_query( t, query ) > 0 ) {
			const char fmt[] = "Failed to run query against selected db and table: %s";
			snprintf( err, errlen, fmt, mysql_error( t ) );
			mysql_close( t );
			return 0;
		}

		// Use the result
		if ( !( db->res = mysql_store_result( t ) ) ) {
			const char fmt[] = "Failed to store results set from last query: %s";
			snprintf( err, errlen, fmt, mysql_error( t ) );
			mysql_close( t );
			return 0;
		}

		// Set both of these
		db->conn = t;
		conn->conn = (void *)db;
	}
#endif

#ifdef BPGSQL_H
	if ( conn->type == DB_POSTGRESQL ) {
		// Switch everything to use this format now
		pgsql_t *db = NULL;

		if ( !( db = malloc( sizeof( pgsql_t ) ) ) || !memset( db, 0, sizeof( pgsql_t ) ) ) {
			const char fmt[] = "Memory constraint encountered initializing Postgres connection.";
			snprintf( err, errlen, fmt );
			return 0;
		}


		// Attempt to open the connection and handle errors
		if ( PQstatus( ( db->conn = PQconnectdb( conn->connstr ) ) ) != CONNECTION_OK ) {
			const char fmt[] = "Couldn't initialize PostsreSQL connection: %s";
			snprintf( err, errlen, fmt, PQerrorMessage( db->conn ) );
			PQfinish( db->conn );
			return 0;
		}

		// Run the query
		if ( !( db->res = PQexec( db->conn, query ) ) ) {
			const char fmt[] = "Failed to run query against selected db and table: '%s'";
			snprintf( err, errlen, fmt, PQerrorMessage( db->conn ) );
			//PQclear( res );
			PQfinish( db->conn );
			return 0;
		}

		// Set both of these
		conn->conn = (void *)db;
	}
#endif
	#if 0
	else if ( conn->type == DB_SQLITE ) {
		sqlite3_t *db = NULL;
		if ( !( db = malloc( sizeof( pgsql_t ) ) ) || !memset( db, 0, sizeof( pgsql_t ) ) ) {
			const char fmt[] = "Memory constraint encountered initializing SQLite3 connection.";
			snprintf( err, errlen, fmt );
			return 0;
		}

		//db->conn = sqlite3_open( );

		conn->conn = (void *)db;
	}
	#endif

	return 1;
}





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
int headers_from_dsn ( dsn_t *conn, char *err, int errlen ) {

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
	else if ( conn->type == DB_SQLITE ) {
		snprintf( err, errlen, "%s", "SQLite3 driver not done yet" );
		return 0;
	}
#endif

#ifdef BMYSQL_H
	else if ( conn->type == DB_MYSQL ) {
		mysql_t *db = (mysql_t *)conn->conn;

		for ( int i = 0, fcount = mysql_field_count( db->conn ); i < fcount; i++ ) {
			MYSQL_FIELD *f = mysql_fetch_field_direct( db->res, i );
			header_t *st = NULL;

			if ( !( st = malloc( sizeof( header_t ) )) || !memset( st, 0, sizeof( header_t ) ) ) {
			//if ( CALLOC_NEW_FAILS( st, header_t ) ) {
				const char fmt[] = "Memory constraints encountered while fetching headers from DSN %s";
				snprintf( err, errlen, fmt, conn->connstr );
				return 0;
			}

			// Get the actual type from the database engine
			//st->ntype = f->type;
			snprintf( st->label, sizeof( st->label ), "%s", f->name );
			add_item( &conn->headers, st, header_t *, &conn->hlen );
		}
	}
#endif

#ifdef BPGSQL_H
	else if ( conn->type == DB_POSTGRESQL ) {
		pgsql_t *db = (pgsql_t *)conn->conn;

		for ( int i = 0, fcount = PQnfields( db->res ); i < fcount; i++ ) {
			const char *name = PQfname( db->res, i );
			header_t *st = NULL;

			if ( !( st = malloc( sizeof( header_t ) ) ) || !memset( st, 0, sizeof( header_t ) ) ) {
			//if ( CALLOC_NEW_FAILS( st, header_t ) ) {
				fprintf( stderr, "Memory constraints encountered while building schema" );
				return 0;
			}

			// Get the actual type from the database engine
			snprintf( st->label, sizeof( st->label ), "%s", name );
			add_item( &conn->headers, st, header_t *, &conn->hlen );
		}
	}
#endif
	else {
		// TODO: Reimplement this to read line-by-line
		zw_t p;
		file_t *file = NULL;

		// Prepare this structure
		memset( &p, 0, sizeof( zw_t ) );

		// Copy the delimiter to whatever this is
		memcpy( &delset[ strlen( delset ) ], DELIM, strlen( DELIM ) );

		// Check anyway in case something wonky happened
		if ( !( file = (file_t *)conn->conn ) ) {
			const char fmt[] = "mmap() src was lost.";
			snprintf( err, errlen, fmt );
			return 0;
		}

		// Walk through and find each column first
		for ( int len = 0; strwalk( &p, file->map, delset ); ) {
			char *t = ( !p.size ) ? no_header : copy( &((char *)file->map)[ p.pos ], p.size - 1, &len, case_type );
			header_t *st = NULL;

			// ...
			if ( !( st = malloc( sizeof( header_t ) ) ) || !memset( st, 0, sizeof( header_t ) ) ) {
				const char fmt[] = "Memory constraints encountered while fetching headers from file '%s'";
				snprintf( err, errlen, fmt, conn->connstr );
				return 0;
			}

			snprintf( st->label, sizeof( st->label ) - 1, "%s", t );
			add_item( &conn->headers, st, header_t *, &conn->hlen );
			free( t );
			if ( p.chr == '\n' || p.chr == '\r' ) break;
		}
	}

	if ( conn->hlen < 2 ) {
		const char fmt[] = "Delimiter ('%s') not found at connection '%s'";
		snprintf( err, errlen, fmt, DELIM, conn->connstr );
		return 0;
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
		fprintf( stdout, "class %s {\n", root );
	}
	else {
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
	// Move up the buffer so that we start from the right offset
	//while ( *(buffer++) != '\n' );

	if ( conn->type == DB_FILE ) {
		file_t *file = (file_t *)conn->conn;
		int len = file->size;
		// Prevent from reading past the end...
		while ( len-- && *((unsigned char *)file->map++) != '\n' );
		file->offset = file->size - len;
	}

	return 1;
}



/**
 * records_from_dsn( dsn_t * )
 * ===========================
 *
 * Extract records from a data source.
 *
 * conn = The source we're reading records from
 * count = Number of "rows" to read at a time
 * offset = Where to start from next
 *
 */
int records_from_dsn( dsn_t *conn, int count, int offset, char *err, int errlen ) {

	if ( conn->type == DB_FILE ) {
		zw_t *p = malloc( sizeof( zw_t ) );
		memset( p, 0, sizeof( zw_t ) );
		column_t **cols = NULL;
		row_t *row = NULL;
		file_t *file = NULL;

		const unsigned int dlen = strlen( delset );
		unsigned int ci = 0;
		unsigned int line = 0;
		unsigned char *dset = (unsigned char *)delset;
		int clen = 0;

		// Cast
		if ( !( file = (file_t *)conn->conn ) ) {
			const char fmt[] = "mmap() lost reference";
			snprintf( err, errlen, fmt );
			return 0;
		}

		// Cycle through the set of entries that we want
		//for ( column_t *col = NULL; memwalk( p, file->map, dset, file->size - file->offset, dlen ) && line < count; ) {
		for ( ; memwalk( p, file->map, dset, file->size - file->offset, dlen ) && line < count; ) {

			// Deifne some stuff to make this easier to walk
			column_t *col = NULL;
			type_t etype = T_NULL;

			// Increase the line count
			if ( p->chr == '\r' ) {
				line++;
				continue;
			}

			// Exit if the column count indicates that this is not uniform data
			if ( ci > conn->hlen ) {
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

			// Get approximate types from the datasource and all of the other info
			etype = conn->headers[ ci ]->type;
			col->k = conn->headers[ ci ]->label;
			col->v = &( (unsigned char *)file->map)[ p->pos ];
			col->len = p->size - 1;

			// Check the actual type against the expected type
			if ( ( col->type = get_type( col->v, etype, col->len ) ) != etype ) {
			//if ( col->type != etype ) {
				// Integers can pass as floats
				if ( col->type == T_INTEGER && etype == T_DOUBLE ) {
					const char fmt[] = "WARNING: Value at row %d, column '%s' (%d). Expected T_DOUBLE got T_INTEGER\n";
					fprintf( stderr, fmt, line + 1, col->k, ci + 1 );
				}
				else if ( col->type == T_INTEGER && etype == T_STRING ) {
					const char fmt[] = "WARNING: Value at row %d, column '%s' (%d). Expected T_STRING got T_INTEGER\n";
					fprintf( stderr, fmt, line + 1, col->k, ci + 1 );
				} 
				// If I'm supposed to have a boolean and it comes back as an INTEGER, this isn't a problem if the value is right
				else if ( col->type == T_INTEGER && etype == T_BOOLEAN ) {
					// You'll have to convert the number and see if it's a 0 or 1
					int check = usafecpynumeric( col->v, col->len ); 
					if ( check > 1 || check < 0 ) {
						const char fmt[] = "Type check for value at row %d, column '%s' (%d) failed. (Expected T_BOOLEAN, got T_INTEGER)";
						snprintf( err, errlen, fmt, line + 1, col->k, ci + 1 );
						return 0;
					} 
					const char fmt[] = "WARNING: Value at row %d, column '%s' (%d). Expected T_BOOLEAN got T_INTEGER\n";
					fprintf( stderr, fmt, line + 1, col->k, ci + 1 );
				}

				// Chars can pass as strings
				else if ( col->type == T_CHAR && etype == T_STRING ) {
					const char fmt[] = "WARNING: Value at row %d, column '%s' (%d). Expected T_STRING got T_CHAR";
					fprintf( stderr, fmt, line + 1, col->k, ci + 1 );
				}

				// Anything else is a failure for now
				else {
					const char fmt[] = "Type check for value at row %d, column '%s' (%d) failed. (Expected %s, got %s)\n";
					snprintf( err, errlen, fmt, line + 1, col->k, ci + 1, itypes[ etype ], itypes[ col->type ] );
					return 0;
				}
			}

			// Add columns and increase column count
			add_item( &cols, col, column_t *, &clen );
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
				add_item( &conn->rows, row, row_t *, &conn->rlen );
				cols = NULL, clen = 0, ci = 0, line++;
			}
		}
		free( p );
	}
#ifdef BSQLITE_H
	else if ( conn->type == DB_SQLITE ) {
		;
	}
#endif
#ifdef BMYSQL_H
#if 0
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
#endif
#ifdef BPGSQL_H
 #if 0
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
			#if 1
				col->v = (unsigned char *)PQgetvalue( conn->res, i, ci );
			#else
				// If the network connection cuts out, it's very possible that this data will be gone
				//col->v = (unsigned char *)strdup( PQgetvalue( conn->res, i, ci ) );
			#endif

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
#endif
#ifdef BPGSQL_H
	char **pgsql_bind_array = NULL;
	int *pgsql_lengths_array = 0;
#endif

	// All of this now belongs in prepare DSN
	// Make the thing
	if ( 0 );
#ifdef BMYSQL_H
	else if ( oconn->type == DB_MYSQL ) {
		mysql_stmt = (void *)mysql_stmt_init( oconn->conn );
#if 1
		mysql_bind_array = malloc( sizeof( MYSQL_BIND ) * iconn->hlen );
		memset( mysql_bind_array, 0, sizeof( MYSQL_BIND ) * iconn->hlen );
#endif
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

	// Most likely this will be needed here
	file_t *file = NULL;
	int first = *row == *iconn->rows;

		//Prefix
	if ( oconn->type == DB_FILE ) {

		file = (file_t *)oconn->conn;

		( FF.prefix ) ? FDPRINTF( file->fd, FF.prefix ) : 0;

		if ( t == STREAM_PRINTF || t == STREAM_COMMA )
			;
		else if ( t == STREAM_XML ) {
			FDPRINTF( file->fd, "<" );
			FDPRINTF( file->fd, oconn->tablename );
			FDPRINTF( file->fd, ">" );
			( FF.newline ) ? FDPRINTF( file->fd, "\n" ) : 0;
		}
		else if ( t == STREAM_CSTRUCT )
			FDPRINTF( file->fd, ( odv ) ? "," : " " ), FDPRINTF( file->fd, "{" );
		else if ( t == STREAM_CARRAY )
			FDPRINTF( file->fd, ( odv ) ? "," : " " ), FDPRINTF( file->fd, "{" );
		else if ( t == STREAM_JSON )
			FDPRINTF( file->fd, ( odv ) ? "," : " " ), FDNPRINTF( file->fd, "{\n", ( FF.newline ) ? 2 : 1 );
		else if ( t == STREAM_SQL ) {
			// Output the table name
			FDPRINTF( file->fd, "INSERT INTO " );
			FDPRINTF( file->fd, oconn->tablename );
			FDPRINTF( file->fd, " (" );

			for ( header_t **x = iconn->headers; x && *x; x++ ) {
				( *iconn->headers != *x ) ? FDPRINTF( file->fd, "," ) : 0;
				FDPRINTF( file->fd, (*x)->label );
			}

			FDPRINTF( file->fd, ") VALUES (" );
		}
	}

	else {
		//else if ( t == STREAM_PGSQL || t == STREAM_MYSQL /* t == STREAM_SQLITE3 */ ) {
	#ifdef BPGSQL_H
		if ( t == STREAM_PGSQL ) {
			// First (and preferably only once) generate the header keys
			char argfmt[ 256 ];
			char *ins = insfmt;
			int ilen = sizeof( insfmt );
			char *s = argfmt;
			const char fmt[] = "INSERT INTO %s ( %s ) VALUES ( %s )";

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
			snprintf( ffmt, sizeof( ffmt ), fmt, iconn->tablename, insfmt, argfmt );
			//fprintf( stdout, "%s\n", ffmt );
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
	}

		int ci = 0;
		// Loop through each column
		for ( column_t **col = (*row)->columns; col && *col; col++, ci++ ) {
			int first = *col == *((*row)->columns);
			if ( t == STREAM_PRINTF ) {
				FDPRINTF( file->fd, (*col)->k );
				FDPRINTF( file->fd, " => " );
				if ( (*col)->type == T_BOOLEAN && memchr( "Tt1", *(*col)->v, 3 ) )
					FDPRINTF( file->fd, "true" );
				else if ( (*col)->type == T_BOOLEAN && memchr( "Ff0", *(*col)->v, 3 ) )
					FDPRINTF( file->fd, "false" );
				else if ( (*col)->type == T_BINARY ) {
					char bin[ 64 ];
					memset( &bin, 0, sizeof( bin ) );
					snprintf( bin, sizeof( bin ) - 1, "%p (%d bytes)", (void *)(*col)->v, (*col)->len );
					FDPRINTF( file->fd, bin );
				}
				else if ( (*col)->type == T_STRING	|| (*col)->type == T_CHAR ) {
					FDPRINTF( file->fd, FF.leftdelim );
					FDNPRINTF( file->fd, (*col)->v, (*col)->len );
					FDPRINTF( file->fd, FF.rightdelim );
				}
				else {
					FDNPRINTF( file->fd, (*col)->v, (*col)->len );
				}
				FDPRINTF( file->fd, "\n" );
			}
			else if ( t == STREAM_XML ) {
				FDPRINTF( file->fd, "\t<" );
				FDPRINTF( file->fd, (*col)->k );
				FDPRINTF( file->fd, ">" );
				FDNPRINTF( file->fd, (*col)->v, (*col)->len );
				FDPRINTF( file->fd, "</" );
				FDPRINTF( file->fd, (*col)->k );
				FDNPRINTF( file->fd, ">\n", ( FF.newline ) ? 2 : 1 );
			}
			else if ( t == STREAM_CARRAY ) {
				//fprintf( oconn->output, &", \"%s\""[ first ], (*col)->v );
				FDPRINTF( file->fd, &", \""[ first ] );
				FDNPRINTF( file->fd, (*col)->v, (*col)->len );
				FDPRINTF( file->fd, "\"" );
			}
			else if ( t == STREAM_COMMA ) {
				//fprintf( oconn->output, &",\"%s\""[ first ], (*col)->v );
				FDPRINTF( file->fd, &",\""[ first ] );
				FDNPRINTF( file->fd, (*col)->v, (*col)->len );
				FDPRINTF( file->fd, "\"" );
			}
			else if ( t == STREAM_JSON ) {
				//fprintf( oconn->output, "\t%c\"%s\": ", first ? ' ' : ',', (*col)->k );
				//FDPRINTF( file->fd, ( first ) ? " " : "," );
				FDPRINTF( file->fd, &",\""[ first ] );
				FDPRINTF( file->fd, (*col)->k );
				FDPRINTF( file->fd, "\": " );

				if ( (*col)->type == T_NULL )
					FDPRINTF( file->fd, "null" );
				else if ( (*col)->type == T_BOOLEAN && (*col)->len && memchr( "Tt1", *(*col)->v, 3 ) )
					FDPRINTF( file->fd, "true" );
				else if ( (*col)->type == T_BOOLEAN && (*col)->len && memchr( "Ff0", *(*col)->v, 3 ) )
					FDPRINTF( file->fd, "false" );
				else if ( (*col)->type == T_STRING || (*col)->type == T_CHAR ) {
					FDPRINTF( file->fd, "\"" );
					FDNPRINTF( file->fd, (*col)->v, (*col)->len );
					FDPRINTF( file->fd, "\"" );
				}
				else {
					FDNPRINTF( file->fd, (*col)->v, (*col)->len );
				}
				( FF.newline ) ? FDPRINTF( file->fd, "\n" ) : 0;
			}
			else if ( t == STREAM_CSTRUCT ) {
				//p_cstruct( 1, *col );
				//check that it's a number
				//char *vv = (char *)(*col)->v;
				//Handle blank values
				FDPRINTF( file->fd, &TAB[9-1] );

			#if 1
				if ( !(*col)->len ) {
					FDPRINTF( file->fd, &", ."[ first ] ),
					FDPRINTF( file->fd, (*col)->k ),
					FDPRINTF( file->fd, " = \"\"" );
				}
				else {
					FDPRINTF( file->fd, &", ."[ first ] ),
					FDPRINTF( file->fd, (*col)->k );
					FDPRINTF( file->fd, " = " );

					if ( (*col)->type == T_INTEGER || (*col)->type == T_DOUBLE )
						FDNPRINTF( file->fd, (*col)->v, (*col)->len );
					else if ( (*col)->type == T_NULL )
						FDPRINTF( file->fd, "NULL" );
					else if ( (*col)->type == T_STRING ) {
						FDPRINTF( file->fd, "\"" ),
						FDNPRINTF( file->fd, (*col)->v, (*col)->len );
						FDPRINTF( file->fd, "\"" );
					}
					else if ( (*col)->type == T_CHAR ) {
						FDPRINTF( file->fd, "'" ),
						FDNPRINTF( file->fd, (*col)->v, (*col)->len );
						FDPRINTF( file->fd, "'" );
					}
					else if ( (*col)->type == T_BINARY ) {
						// TODO: Write a hex stream
						FDPRINTF( file->fd, "\"" ),
						FDNPRINTF( file->fd, (*col)->v, (*col)->len );
						FDPRINTF( file->fd, "\"" );
					}
					else {
						// Assume a string
						FDPRINTF( file->fd, " = \"" );
						FDNPRINTF( file->fd, (*col)->v, (*col)->len );
						FDPRINTF( file->fd, "\"" );
					}
				}
				FDPRINTF( file->fd, "\n" );
			#else
				if ( !strlen( vv ) ) {
					fprintf( oconn->output, "%s", &TAB[9-1] );
					fprintf( oconn->output, &", .%s = \"\"\n"[ first ], (*col)->k );
				}
				else {
					fprintf( oconn->output, "%s", &TAB[9-1] );
					fprintf( oconn->output, &", .%s = \"%s\"\n"[ first ], (*col)->k, (*col)->v );
				}
			#endif
			}
			else if ( t == STREAM_SQL ) {
				( first ) ? 0 : FDPRINTF( file->fd, "," );

				if ( (*col)->type != T_STRING && (*col)->type != T_CHAR )
					FDNPRINTF( file->fd, (*col)->v, (*col)->len );
				else {
					FDPRINTF( file->fd, FF.leftdelim );
					FDNPRINTF( file->fd, (*col)->v, (*col)->len );
					FDPRINTF( file->fd, FF.rightdelim );
				}
#if 0
				if ( !typesafe ) {
//fprintf( oconn->output, &",%s%s%s"[ first ], ld, (*col)->v, rd );
					( first ) ? FDPRINTF( file->fd, "," ) : 0;
					FDPRINTF( file->fd, ld );
					FDNPRINTF( file->fd, (*col)->v, (*col)->len );
					FDPRINTF( file->fd, rd );
				}
				else {
					// Should check that they match too
					if ( (*col)->type != T_STRING && (*col)->type != T_CHAR )
						fprintf( oconn->output, &",%s"[ first ], (*col)->v );
					else {
						fprintf( oconn->output, &",%s%s%s"[ first ], ld, (*col)->v, rd );
					}
				}
#endif
			}
		#ifdef BMYSQL_H
			else if ( t == STREAM_MYSQL ) {
			#ifdef DEBUG_H
				// We definitely do use this...
				header_t **headers = iconn->headers;
				//fprintf( stderr, "Binding value %p, %d of length %d\n", (*col)->v, (headers[ci])->ntype,(*col)->len );
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

		// End the string
		if ( t == STREAM_PRINTF )
			;//p_default( 0, ib->k, ib->v );
		else if ( t == STREAM_XML ) {
			FDPRINTF( file->fd, "</" );
			FDPRINTF( file->fd, oconn->tablename );
			FDPRINTF( file->fd, ">" );
		}
		else if ( t == STREAM_COMMA )
			;//fprintf( oconn->output, " },\n" );
		else if ( t == STREAM_CSTRUCT )
			FDPRINTF( file->fd, "}" );
		else if ( t == STREAM_CARRAY )
			FDPRINTF( file->fd, " }" );
		else if ( t == STREAM_SQL )
			FDPRINTF( file->fd, " );" );
		else if ( t == STREAM_JSON ) {
			//wipe the last ','
			FDNPRINTF( file->fd, "}", 1 );
			//FDPRINTF( file->fd, "," );
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

		//DPRINTF( "Completed row: %d", ri ), fsync( 2 ), getchar();

		//Suffix
		if ( oconn->type == DB_FILE ) {
			( FF.suffix ) ? FDPRINTF( file->fd, FF.suffix ) : 0;
			( FF.newline ) ? FDPRINTF( file->fd, "\n" ) : 0;
		}

	}


	// All of this crap should now be moved to teardown or unprepare_dsn()
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

#if 0
	// If the stream is a file, flush it
	if ( t != STREAM_MYSQL && t != STREAM_PGSQL ) {
		fsync( file->fd );
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

	if ( conn->type == DB_FILE ) {
		// Cast
		file_t *file = (file_t *)conn->conn;

		// Free this
		if ( file->start && munmap( (void *)file->start, file->size ) == -1 ) {
			fprintf( stderr, "munmap() %s\n", strerror( errno ) );
		}

		// Close if the file is not stdout or in
		if ( file->fd > 2 && close( file->fd ) == -1 ) {
			fprintf( stderr, "close() %s\n", strerror( errno ) );
		}

		free( file );
	}
#ifdef BSQLITE_H
	else if ( conn->type == DB_SQLITE )
		fprintf( stderr, "SQLite3 not done yet.\n" );
#endif
#ifdef BMYSQL_H
	else if ( conn->type == DB_MYSQL ) {
		mysql_t *db = ( mysql_t *)conn->conn;
		mysql_free_result( db->res );
		mysql_close( (MYSQL *)db->conn );
		free( db );
	}
#endif
#ifdef BPGSQL_H
	else if ( conn->type == DB_POSTGRESQL ) {
		pgsql_t *db = ( pgsql_t *)conn->conn;
		PQclear( db->res );
		PQfinish( (PGconn *)db->conn );
		free( db );
	}
#endif

	free( conn->connstr );
	conn->conn = NULL;
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
	char *ename = NULL;
	coerce_t **ctypes = NULL;
	const typemap_t *cdefs = NULL;

	// Evaluate the coercion string if one is available
	// The type that is saved will be the coerced one
	if ( ov ) {
		int i = 0;
		int ctypeslen = 0;

		// Multiple coerced types
		if ( memchr( ov, ',', strlen( ov ) ) ) {
			zw_t p = { 0 };
			for ( coerce_t *t = NULL; strwalk( &p, ov, "," ); t = NULL ) {
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

		// User called this wrong
		else {
			snprintf( err, errlen, "Argument supplied to --coerce is invalid.\n" );
			return 0;
		}

		// Better check the coerced columns against the columns
		for ( coerce_t **c = ctypes; c && *c; c++ ) {
			int i = 0;

			for ( header_t **t = iconn->headers; !i && t && *t; t++ )
				if ( !strcmp( (*c)->name, (*t)->label ) ) i++; 

			if ( !i ) {
				const char fmt[] = "Requested type coercion on column '%s', not found in datasource.";	
				snprintf( err, errlen, fmt, (*c)->name );
				return 0;
			}
		}
	}

	//
	if ( iconn->type == DB_FILE ) {
		zw_t p;
		int count = 0;
		unsigned int rowlen = 0;
		unsigned int dlen = strlen( delset );
		file_t *file = NULL;

		// Catch any silly errors that may have been made on the way back here
		if ( !( file = (file_t *)iconn->conn ) ) {
			const char fmt[] = "mmap() lost reference";
			snprintf( err, errlen, fmt );
			return 0;
		}

		// Find the end of the first row
		for ( unsigned char *s = file->map; ( *s != '\n' && *s != '\r' ); s++ ) {
			rowlen++;
		}

		// Check that col count - 1 == occurrence of delimiters in file
		if ( ( iconn->hlen - 1 ) != ( count = memstrocc( file->map, &delset[2], rowlen ) ) ) {
			const char fmt[] = "Header count and column count differ in file: %s (%d != %d)";
			snprintf( err, errlen, fmt, iconn->connstr, iconn->hlen, count );
			return 0;
		}

		// Initialize buffer
		memset( &p, 0, sizeof( zw_t ) );

		// Move through the rest of the entries in the row and approximate types
		for ( int i = 0; memwalk( &p, file->map, (unsigned char *)delset, rowlen, dlen ); i++ ) {
			header_t *st = iconn->headers[ i ];
			char *t = NULL;
			const char *ctype = NULL;
			int len = 0;

			// Check if any of these are looking for a coerced type
			if ( ov && ( ctype = find_ctype( ctypes, st->label ) ) ) {
				if ( !( st->ctype = get_typemap_by_nname( oconn->typemap, ctype ) ) ) {
					const char fmt[] = "FIL: Failed to find desired type '%s' at column '%s' for "
						"supported %s types ";
					snprintf( err, errlen, fmt, ctype, st->label, get_conn_type( oconn->type ) );
					return 0;
				}
			}

			// TODO: Assuming a blank value is a string might not always be the best move
			if ( !p.size ) {
				st->type = T_STRING;
				continue;
			}

			// Try to copy again
			if ( !( t = copy( &((char *)file->map)[ p.pos ], p.size - 1, &len, CASE_NONE ) ) ) {
				// TODO: This isn't very clear, but it is unlikely we'll ever run into it...
				snprintf( err, errlen, "Out of memory error occurred." );
				return 0;
			}

			// Handle NULL or empty strings
			if ( !len ) {
				st->type = T_STRING;
				continue;
			}

			st->type = get_type( (unsigned char *)t, T_STRING, len );
			free( t );
		}
	}
#ifdef BSQLITE_H
	else if ( iconn->type == DB_SQLITE ) {
		snprintf( err, errlen, "%s", "SQLite3 driver not done yet" );
		return 0;
	}
#endif

#ifdef BMYSQL_H
	else if ( iconn->type == DB_MYSQL ) {
		mysql_t *b = (mysql_t *)iconn->conn;

		for ( int i = 0, fcount = mysql_field_count( b->conn ); i < fcount; i++ ) {
			MYSQL_FIELD *f = mysql_fetch_field_direct( b->res, i );
			header_t *st = iconn->headers[ i ];
			const char *ctype = NULL;

			// Get the actual type from the database engine and make sure it's supported
			if ( !( st->ntype = get_typemap_by_ntype( iconn->typemap, f->type ) ) ) {
				snprintf( err, errlen, "Unsupported MySQL type received: %d", f->type );
				return 0;
			}

			// If the user asked for a coerced type, add it here
			if ( ov && ( ctype = find_ctype( ctypes, f->name ) ) ) {
				// Find the forced/coerced type or die trying
fprintf( stdout, "typemap = %p\n", (void *)oconn->typemap );  
				if ( !( st->ctype = get_typemap_by_nname( oconn->typemap, ctype ) ) ) {
					const char fmt[] = "MY: Failed to find desired type '%s' for column '%s' "
						"for supported %s types ";
					snprintf( err, errlen, fmt, ctype, f->name, ename );
					return 0;
				}
			}

			// Set the base type and write the column name
			st->type = st->ntype->basetype;
		#if 0
			st->maxlen = 0;
			st->precision = 0;
			st->filter = 0;
			st->date = 0; // The timezone data?
		#endif
		}
	}
#endif

#ifdef BPGSQL_H
	else if ( iconn->type == DB_POSTGRESQL ) {
		pgsql_t *b = (pgsql_t *)iconn->conn;
		for ( int i = 0, fcount = PQnfields( b->res ); i < fcount; i++ ) {
			header_t *st = iconn->headers[ i ];
			const char *ctype = NULL;
			const char *name = PQfname( b->res, i );
			Oid pgtype = PQftype( b->res, i );

			// Get the actual type from the database engine and make sure it's supported
			if ( !( st->ntype = get_typemap_by_ntype( iconn->typemap, (int)pgtype ) ) ) {
				snprintf( err, errlen, "Unsupported Postgres type received: %d", pgtype );
				return 0;
			}

#if 1
			// If the user asked for a coerced type, add it here
			if ( ov && ( ctype = find_ctype( ctypes, name ) ) ) {
				// Find the forced/coerced type or die trying
				if ( !( st->ctype = get_typemap_by_nname( oconn->typemap, ctype ) ) ) {
					const char fmt[] = 
						"PG: Failed to find desired type '%s' for column '%s' in supported %s types";
					snprintf( err, errlen, fmt, ctype, name, "Postgres" );
					return 0;
				}
			}
#endif
			// Set the base type and write the column name
			st->type = st->ntype->basetype;
		#if 0
			st->maxlen = 0;
			st->precision = 0;
			st->filter = 0;
			st->date = 0; // The timezone data?
		#endif
		}

	}
#endif

	for ( coerce_t **x = ctypes; x && *x; x++ ) free( *x );
	free( ctypes );
	return 1;
}



/**
 * scaffold_dsn( dsn_t *, dsn_t *, const char * )
 * ==============================================
 *
 * This is intended to fill out a dsn_t manually when we already have an input 
 * source.
 *
 */
int scaffold_dsn ( config_t *conf, dsn_t *ic, dsn_t *oc, char *tname, char *err, int errlen  ) {

	// Define stuffs
	char *t = NULL;

	// No connection string was specified, so we can make a few assumptions.
	if ( !oc->connstr ) {
		// Output source is going to be stdout
		oc->type = DB_FILE;
		oc->connstr = strdup( "/dev/stdout" );
		oc->stream = STREAM_PRINTF;
		oc->typemap = default_map;

		if ( tname ) {
			if ( strlen( tname ) >= sizeof( oc->tablename ) ) {
				snprintf( err, errlen, "Table name '%s' is too long for buffer.", tname );
				return 0;
			}
			snprintf( oc->tablename, sizeof( oc->tablename ), "%s", tname );
		}
		else if ( strlen( ic->tablename ) > 0 ) {
			snprintf( oc->tablename, sizeof( oc->tablename ), "%s", ic->tablename );
		}

		// I'll need a table name depending on other options
	#if 0
		if ( config->wschema && !config->tname && !strlen( ic->tablename ) ) {
			snprintf( err, errlen, "Wanted --schema, but table name is invalid or unspecified." );
			return 0;
		}
	#endif

		// If we're reading from a database input source make some assumptions
		if ( ic->type == DB_MYSQL )
			oc->typemap = mysql_map;
		else if ( ic->type == DB_POSTGRESQL )
			oc->typemap = pgsql_map;
		else {
			oc->typemap = default_map;
		}	

		// There is no real need to parse here.
		return 1;
	}


	// TODO: Might need to parse output dsn twice...
	if ( !parse_dsn_info( oc, err, errlen ) ) {
		return 0;
	}


	// A cli table name takes highest priority
	if ( tname /* config->tname */ ) {
		if ( strlen( tname ) >= sizeof( oc->tablename ) ) {
			snprintf( err, errlen, "Table name '%s' is too long for buffer.", tname );
			return 0;
		}
		snprintf( oc->tablename, sizeof( oc->tablename ), "%s", tname );
	}

	// Copy the ic->tn to the oc->tn
	else if ( !strlen( oc->tablename ) && strlen( ic->tablename ) > 0 ) {
		snprintf( oc->tablename, sizeof( oc->tablename ), "%s", ic->tablename );
	}

	
	// Check if it's NOT a file, and then make some decisions
	if ( !oc->type || oc->type == DB_NONE ) {
		snprintf( err, errlen, "We shouldn't have gotten here..." );
		return 0;
	}
	else if ( oc->type != DB_FILE && !strlen( oc->tablename ) ) {
		snprintf( err, errlen, "Writing to a database, but no table specified." );
		return 0;
	}
	else if ( oc->type == DB_FILE ) {
		// If we're reading from a database input source make some assumptions
		if ( ic->type == DB_MYSQL )
			oc->typemap = mysql_map;
		else if ( ic->type == DB_POSTGRESQL )
			oc->typemap = pgsql_map;
		else {
			oc->typemap = default_map;
		}	
	}
	else {
		// If we're reading from db to db, also make some smart assumptions
		if ( oc->type == DB_MYSQL )
			oc->typemap = mysql_map;
		else if ( oc->type == DB_POSTGRESQL )
			oc->typemap = pgsql_map;
		else {
			oc->typemap = default_map;
		}	
	}


	return 1;
} 




/**
 * cmd_headers ( dsn_t * )
 * ===========================
 *
 * Dump the headers only and stop.
 *
 */
int cmd_headers ( dsn_t *conn ) {
	for ( header_t **s = conn->headers; s && *s; s++ )
		printf( "%s\n", (*s)->label );

	// Free everything
	destroy_dsn_headers( conn );
	close_dsn( conn );
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





static config_t config = {
	.wstream = 0,	// wstream - A chosen stream
	.wdeclaration = 0,	// wdeclaration - Dump a class or struct declaration only 
	.wschema = 0,	// wschema - Dump the schema only
	.wheaders = 0,	// wheaders - Dump the headers only
	.wconvert = 0,	// wconvert - Convert the source into some other format
	.wclass = 0,	// wclass - Convert to a class
	.wstruct = 0,	// wstruct - Convert to a struct
	.wtypesafe = 0,
	.wnewline = 0,
	.wdatestamps = 0,
	.wnounsigned = 0,
	.wspcase = 0,
	.wid = 0,   // Use a unique ID when reading from a datasource
	.wstats = 0,   // Use the stats
	
	.wcutcols = NULL,  // The user wants to cut columns
	.widname = NULL,  // The unique ID column name
	.wcoerce = NULL,  // The user wants to override certain types
	.wquery = NULL,	// A custom query from the user
	.wprefix = NULL,
	.wsuffix = NULL, 
	.wtable = NULL

};



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
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --coerce." );
			}
		}
		else if ( !strcmp( *argv, "--id" ) ) {
			want_id = 1;
			if ( !dupval( *( ++argv ), &id_name ) ) {
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --id." );
			}
		}
		else if ( !strcmp( *argv, "--for" ) ) {
			if ( output.connstr )
				return ERRPRINTF( ERRCODE, "%s\n", "Can't specify both --for & --output" );

			if ( *( ++argv ) == NULL )
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for flag --for." );

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
				return ERRPRINTF( ERRCODE, "Oracle SQL dialect is currently unsupported, sorry...\n" );
			}
		#endif
			else {
				return ERRPRINTF( ERRCODE, "Argument specified '%s' for flag --for is an invalid option.", *argv );
			}

		}
		else if ( !strcmp( *argv, "-f" ) || !strcmp( *argv, "--format" ) ) {
			convert = 1;
			if ( !dupval( *( ++argv ), &streamtype ) )
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --FF." );
			else if ( ( stream_fmt = check_for_valid_stream( streamtype ) ) == -1 ) {
				return ERRPRINTF( ERRCODE, "Invalid format '%s' requested.\n", streamtype );
			}
		}
		else if ( !strcmp( *argv, "--class" ) ) {
			classdata = 1;
			serialize = 1;
			case_type = CASE_CAMEL;
#if 0
			if ( !dupval( *( ++argv ), &input.connstr ) ) {
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --class." );
			}
#endif
		}
		else if ( !strcmp( *argv, "--struct" ) ) {
			structdata = 1;
			serialize = 1;
#if 0
			if ( !dupval( *( ++argv ), &input.connstr ) ) {
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --struct." );
			}
#endif
		}
		else if ( !strcmp( *argv, "-p" ) || !strcmp( *argv, "--prefix" ) ) {
			if ( !dupval( *(++argv), &prefix ) )
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --prefix." );
		}
		else if ( !strcmp( *argv, "-x" ) || !strcmp( *argv, "--suffix" ) ) {
			if ( !dupval( *(++argv), &suffix ) )
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --suffix." );
		}
		else if ( !strcmp( *argv, "-d" ) || !strcmp( *argv, "--delimiter" ) ) {
			char *del = *(++argv);
			if ( !del )
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --delimiter." );
			else if ( *del == 9 || ( strlen( del ) == 2 && !strcmp( del, "\\t" ) ) )
				DELIM = "\t";
			else if ( !dupval( del, &DELIM ) ) {
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --delimiter." );
			}
		}
		else if ( !strcmp( *argv, "-u" ) || !strcmp( *argv, "--output-delimiter" ) ) {
			//Output delimters can be just about anything,
			//so we don't worry about '-' or '--' in the statement
			if ( ! *( ++argv ) )
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --output-delimiter." );
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
		else if ( !strcmp( *argv, "-T" ) || !strcmp( *argv, "--table" ) ) {
			if ( !dupval( *(++argv), &table ) )
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --table." );
		}
		else if ( !strcmp( *argv, "-Q" ) || !strcmp( *argv, "--query" ) ) {
			if ( !dupval( *(++argv), &query ) )
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --query." );
		}
		else if ( !strcmp( *argv, "-i" ) || !strcmp( *argv, "--input" ) ) {
			if ( !dupval( *( ++argv ), &input.connstr ) )
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --input." );
		}
		else if ( !strcmp( *argv, "-o" ) || !strcmp( *argv, "--output" ) ) {
			char *arg = *( ++argv );
			if ( !arg || !dupval( arg, &output.connstr ) ) {
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --output." );
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

	// Start the timer here
	if ( show_stats )
		clock_gettime( CLOCK_REALTIME, &stimer );

	// If the user didn't actually specify an action, we should close and tell them that
	// what they did is useless.
	if ( !convert && !schema && !headers_only )
		return ERRPRINTF( ERRCODE, "No action specified, exiting.\n" );

	// The dsn is always going to be the input
	if ( !parse_dsn_info( &input, err, sizeof( err  ) ) )
		return ERRPRINTF( ERRCODE, "Input source is invalid: %s\n", err );

	// If the user specified a table, fill out 
	if ( table && strlen( table ) < sizeof( input.tablename ) )
		snprintf( input.tablename, sizeof( input.tablename ), "%s", table );

	// Open the DSN first (regardless of type)
	if ( !open_dsn( &input, query, err, sizeof( err ) ) )
		return ERRPRINTF( ERRCODE, "DSN open failed: %s.\n", err );

	// Create the header_t here
	if ( !headers_from_dsn( &input, err, sizeof( err ) ) ) {
		close_dsn( &input );
		return ERRPRINTF( ERRCODE, "Header creation failed: %s.\n", err );
	}

	// Create the headers only
	if ( headers_only )
		return cmd_headers( &input );	

	// Scaffolding can fail
	if ( !scaffold_dsn( &config, &input, &output, table, err, sizeof( err ) ) )
		return ERRPRINTF( ERRCODE, "%s", err );

	// The input datasource has to be prepared too
	if ( !prepare_dsn( &input, err, sizeof( err ) ) ) {
		destroy_dsn_headers( &input );
		close_dsn( &input );
		return ERRPRINTF( ERRCODE, "Failed to prepare DSN: %s.\n", err );
	}

	// This should assert, but we check it anyway 
	if ( !input.typemap || !output.typemap ) {
		destroy_dsn_headers( &input );
		close_dsn( &input );
		return ERRPRINTF( ERRCODE, "Failed to prepare DSN: %s.\n", err );
	}

	// Get the types of each thing in the column
	if ( !types_from_dsn( &input, &output, coercion, err, sizeof( err ) ) ) {
		return ERRPRINTF( ERRCODE, "typeget failed: %s\n", err );
	}

#if 0
fprintf( stdout, "INPUT CS: %s\n", input.connstr ), 
	print_dsn( &input ),
fprintf( stdout, "OUTPUT CS: %s\n", output.connstr ), 
	print_dsn( &output );

for ( const typemap_t *t = output.typemap; t->ntype != TYPEMAP_TERM; t++ )
	printf( "typename: %s (%s)\n", t->typename, t->libtypename );
#endif

	// Create a schema
	if ( schema ) {
		if ( !schema_from_dsn( &input, &output, schema_fmt, MAX_STMT_SIZE, err, sizeof( err ) ) ) {
			free( output.connstr );
			destroy_dsn_headers( &input );
			close_dsn( &input );
			return ERRPRINTF( ERRCODE, "Schema build failed: %s\n", err );
		}

		// Dump said schema
		fprintf( stdout, "%s", schema_fmt );

		// Destroy the output string
		free( output.connstr );
	}

#if 0
	else if ( classdata || structdata ) {
		DPRINTF( "classdata || structdata only was called...\n" );
		//if ( !struct_f( FFILE, DELIM ) ) return 1;
	}
#endif

	else if ( convert ) {
#if 0
fprintf( stdout, "INPUT CS: %s\n", input.connstr ), 
	print_dsn( &input ),
fprintf( stdout, "OUTPUT CS: %s\n", output.connstr ), 
	print_dsn( &output );
#endif

	#if 1
		// Test and try to create if it does not exist
		if ( !test_dsn( &output, err, sizeof( err ) ) && !create_dsn( &input, &output, err, sizeof( err ) ) ) {
			destroy_dsn_headers( &input );
			close_dsn( &input );
			return ERRPRINTF( ERRCODE, "Output DSN '%s' is inaccessible or could not be created: %s\n", output.connstr, err );
			close_dsn( &output );
			return 1;
		}
	#else
		// Create the output DSN if it does not exist
		if ( !create_dsn( &output, &input, err, sizeof( err ) ) ) {
			fprintf( stderr, "Creating output DSN failed: %s\n", err );
			return 1;
		}
	#endif

		// Open the output dsn
		if ( !open_dsn( &output, query, err, sizeof( err ) ) ) {
			destroy_dsn_headers( &input );
			close_dsn( &input );
			return ERRPRINTF( ERRCODE, "DSN open failed: %s\n", err );
			return 1;
		}

		// Control the streaming / buffering from here
		for ( int i = 0; i < 1; i++ ) {

			// Stream to records
			if ( !records_from_dsn( &input, 100, 0, err, sizeof( err ) ) ) {
				destroy_dsn_headers( &input );
				close_dsn( &input );
				return ERRPRINTF( ERRCODE, "Failed to generate records from data source: %s.\n", err );
			}

			// Do the transport (buffering can be added pretty soon)
			if ( !transform_from_dsn( &input, &output, stream_fmt, 0, 0, err, sizeof(err) ) ) {
				fprintf( stderr, "Failed to transform records from data source: %s\n", err );
				destroy_dsn_headers( &input );
				close_dsn( &input );
				return 1;
			}

			// Destroy the rows
			destroy_dsn_rows( &input );
		} // end for

		// once we get to the end, "dismount" whatever preparations we made
		//freesupp_dsn( &input );

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

	// Destroy the headers
	destroy_dsn_headers( &input );

	// Close the DSN
	close_dsn( &input );
	return 0;
}
