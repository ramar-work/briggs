/**
 * briggs.c
 * ========
 *
 * A tool for converting CSV sheets into other data formats.
 *
 *
 * Usage
 * -----
 * briggs can be used to turn CSV data or SQL query results 
 * into: 
 * - a SQL query or transaction 
 * - a C-style struct
 * - JSON
 * - XML
 * - Java classes, arrays or objects
 * - Dart classes, arrays or objects
 *
 * briggs can also be used to copy records directly from one
 * source into a live database connection.
 *
 *
 * TODO / Wishlist
 * ---------------
 * - Bring back struct creation
 * - Refactor main() into better modules
 *   - Functions should be used for specific functionality 
 *     (like convert or schema) 
 * - Read DIRECTLY from ods files
 * - Read DIRECTLY from xls/xlsx files
 * - Read from XML, JSON or Msgpack streams as if they were a 
 *   CSV
 * - Give Unicode another look 
 * - Generate random values for a column
 * ~ Add the --auto flag to automatically coerce types in cases
 *   where needed.
 * - Work on offset and count to give the ability to process only
 *   a subset of rows WITHOUT having to write a custom query
 * - Open the code to run on Windows
 * - Add ability to generate random test data
 * 	 - Numbers
 * 	 - Zips
 * 	 - Addresses
 * 	 - Names
 * 	 - Words
 * 	 - Paragraphs
 * 	 - URLs
 * 	 - Images (probably from a directory is the most useful)
 *
 **/
#define _POSIX_C_SOURCE 200809L
#define VERSION "0.01"
#define TAB "\t\t\t\t\t\t\t\t\t"
#define ERRLEN 2048
#define NODES "nodes"

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

/* Can compile this to be bigger if need be */
#define MAX_STMT_SIZE 2048

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

/* Evaluate arguments */
#define EVALARG(x, SHORT, LONG) ( !strcmp( x, SHORT ) || !strcmp( x, LONG ) )

/* Save arguments */
#define SAVEARG(x,v) ( v = *( ++x ) )

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
 #define print_date(A) 0
 #define print_ctypes(A) 0
 #define WHERE() 0
 #define WPRINTF( ... ) 0 
 #define DEVAL( ... ) 0 
#else
	/* Optionally print arguments */
 #define DPRINTF( ... ) fprintf( stderr, __VA_ARGS__ )

	/* Let me know where the program is */
 #define WHERE() \
 	DPRINTF( "%s: [%s:%d]\n", __func__, __FILE__, __LINE__ )

 #define WPRINTF(...) \
 	DPRINTF( "%s: [%s:%d] ", __func__, __FILE__, __LINE__ ) && DPRINTF( __VA_ARGS__ ) && DPRINTF( "\n" )

	/* Print the literal name of a type */
 #define N(A) #A

	/* Run whatever code is inside of this define */
 #define DEVAL( ... ) __VA_ARGS__ 
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
, STREAM_RAW /* Like SQL dump, but with custom row and column delims */
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
	int stricttypes;
	char *prefix;
	char *suffix;
	char *leftdelim;
	char *rightdelim;
} format_t;

static format_t FF = { 1, 0, 0, 0, 0, NULL, NULL, "'", "'" };


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
	T_DATE, //- Conversion to a base type (via tv_sec, tv_nsec ) might make the most sense)
} type_t;


/**
 * typedef struct date_t 
 *
 * Stream dates to this for easy transfer across databases.
 *
 */
typedef struct date_t {
	unsigned short year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
	unsigned int micro;
	unsigned int timezone; // Should probably be a datatype
} date_t;


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
	const char *libname;
	const char *name;
	type_t basetype;
	unsigned long size;
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
	unsigned char *map;  // makes reading and writing much easier
	const void *start;  // free this at the end
	unsigned long size; // the file's size
	unsigned int offset;
// int limit;
} file_t;


/**
 * typedef struct mysql_t 
 *
 * Collection of common MySQL structures.
 *
 */
typedef struct mysql_t {
	MYSQL *conn;
	MYSQL_STMT *stmt;
	MYSQL_BIND *bindargs;
	MYSQL_RES *res;
	const char *query;
} mysql_t;


/**
 * typedef struct pgsql_t 
 *
 * Collection of common PostgreSQL structures.
 *
 */
typedef struct pgsql_t {
	PGconn *conn;
	PGresult *res;
	char **bindargs;
	int *bindlens;
	Oid *bindoids;
	int *bindfmts; // the smallest I can think of
	int arglen;	
	const char *query;
} pgsql_t;


#if 0
// TODO: If we ever add the SQLite backend support...
typedef struct sqlite3_t {
	sqlite3 *conn;
	sqlite3_stmt *stmt;
} sqlite3_t;
#endif


/**
 * typedef enum bindtype_t
 *
 * Different database engines use different methods of binding
 * arguments.  Collect those here.
 *
 */
typedef enum {
	BINDTYPE_NONE,
	BINDTYPE_ALPHA,
	BINDTYPE_NUMERIC,
	BINDTYPE_ANON,
	BINDTYPE_AT	
} bindtype_t;


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

	/* A base type */
	type_t type;

	/* The native matched type */
	typemap_t *ntype;
	//int ntype;

	/* A coerced type (if asked for) */
	typemap_t *ctype;
	//int ctype;

	/* The preferred type to use */
	typemap_t *ptype;

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
	unsigned long len;
	date_t date;
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

	char *rowd;
	char *cold;
} dsn_t;



/**
 * typedef struct function_t 
 *
 * Data structure to contain common functions for different 
 * data source types.
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


/**
 * typedef struct coerce_t 
 *
 * Little structure to contain coerced arguments and their types.
 * 
 * NOTE: There is some wasted space, but this is so much simpler...
 *
 */
typedef struct coerce_t {
	const char misc[ 128 ];
	const char typename[ 128 ];
} coerce_t;


/**
 * typedef struct config_t 
 *
 * User arguments and preferred options.
 *
 */
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
	char wstats;   // Use the stats
	char *wid;   // Use a unique ID when reading from a datasource
	char *wcutcols;  // The user wants to cut columns
	char *widname;  // The unique ID column name
	char *wcoerce;  // The user wants to override certain types
	char *wautocast;  // The user wants to override certain types according to one specific one
	char *wquery;	// A custom query from the user
	char *wprefix;
	char *wsuffix; 
	char *wtable; 
	char *wstype; 
	sqltype_t wdbengine; 
	char *wrowd;
	char *wcold;
#ifdef DEBUG_H
	char wdumpdsn;
#endif

} config_t;


/* --Data */
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


static const char * itypes[] = {
	[T_NULL] = "T_NULL",
	[T_CHAR] = "T_CHAR",
	[T_STRING] = "T_STRING",
	[T_INTEGER] = "T_INTEGER",
	[T_DOUBLE] = "T_DOUBLE",
	[T_BOOLEAN] = "T_BOOLEAN",
	[T_BINARY] = "T_BINARY",
	[T_DATE] = "T_DATE",
};


const char * bindtypes[] = {
	NULL,
	":%s",	
	"$%d",	
	"?",	
	"@%s",	
};


// This is for default types.  A map IS a lot of overhead...
const typemap_t default_map[] = {
	{ T_NULL, N(T_NULL), NULL, T_NULL, 0, 0 },
	{ T_STRING, N(T_STRING), "TEXT", T_STRING, -1, 1 },
	/* NOTE: This will force STRING to take precedence over CHAR.  DO NOT TOUCH THIS */
	{ T_CHAR, N(T_CHAR), "TEXT", T_CHAR, sizeof( char ), 1 },
	{ T_INTEGER, N(T_INTEGER), "INTEGER", T_INTEGER, 0, 1 },
	{ T_DOUBLE, N(T_DOUBLE), "REAL", T_DOUBLE, 0, 1 },
	{ T_BINARY, N(T_BINARY), "BLOB", T_BINARY, 0, 1 },
	{ T_BOOLEAN, N(T_BOOLEAN), "BOOLEAN", T_BOOLEAN, 0, 1 },
	{ T_DATE, N(T_DATE), "DATE", T_DATE, 0, 1 },

	/* Supporting base dates is EXTREMELY difficult... be careful supporting it */
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
#define PG_INTERVALOID 1186

static const typemap_t pgsql_map[] = {
	{ PG_INT2OID, N(PG_INT2OID), "smallint", T_INTEGER, sizeof( short ), 0 },
	{ PG_INT4OID, N(PG_INT4OID), "int", T_INTEGER, sizeof( int ), 1, },
	{ PG_INT4OID, N(PG_INT4OID), "integer", T_INTEGER, sizeof( int ), 0, },
	{ PG_INT8OID, N(PG_INT8OID), "bigint", T_INTEGER, sizeof( int ), 0 },
	{ PG_NUMERICOID, N(PG_NUMERICOID), "numeric", T_DOUBLE, sizeof( double ),  0 },
	{ PG_FLOAT4OID, N(PG_FLOAT4OID), "real", T_DOUBLE, sizeof( double ), 1 }, /* or float4 */
	{ PG_FLOAT8OID, N(PG_FLOAT8OID), "double precision", T_DOUBLE, sizeof( double ), 0 }, /* or float8 */
	{ PG_TEXTOID, N(PG_TEXTOID), "text", T_STRING, 0, 1 },
	{ PG_BPCHAROID, N(PG_BPCHAROID), "varchar", T_STRING, 0, 0 },
	{ PG_VARCHAROID, N(PG_VARCHAROID), "varchar", T_CHAR, 0, 1 },
	{ PG_BYTEAOID, N(PG_BYTEAOID), "bytea", T_BINARY, 0, 1 },
	{ PG_BYTEAOID, N(PG_BYTEAOID), "blob", T_BINARY, 0, 0 },
	{ PG_BOOLOID, N(PG_BOOLOID), "boolean", T_BOOLEAN, 0, 1 },
	{ PG_DATEOID, N(PG_DATEOID), "date", T_DATE, 0 },
	{ PG_TIMEOID, N(PG_TIMEOID), "time", T_DATE, 0 },
	{ PG_TIMESTAMPOID, N(PG_TIMESTAMPOID), "timestamp", T_DATE, 0, 1 },
#if 0
	{ PG_INTERVALOID, N(PG_INTERVALOID), "interval", T_DATE },
	{ PG_DATETIMEOID, N(PG_DATETIMEOID), "datetime", T_DATE },
	{ PG_TIMESTAMPZOID, N(PG_TIMESTAMPZOID), "timestamptz", T_DATE },
	{ PG_TIMETZOID, N(PG_TIMETZOID), "timetz", T_DATE },
#endif
#if 0
	[PG_OIDOID] = "PG_OIDOID",
	[PG_BITOID] = "PG_BITOID",
	[PG_VARBITOID] = "PG_VARBITOID",
#endif
	{ TYPEMAP_TERM },
};
#endif



#ifdef BMYSQL_H
static const typemap_t mysql_map[] = {
	{ MYSQL_TYPE_TINY, N(MYSQL_TYPE_TINY), "TINYINT", T_INTEGER, sizeof( short ) },
	{ MYSQL_TYPE_SHORT, N(MYSQL_TYPE_SHORT), "SMALLINT", T_INTEGER, sizeof( int ) },
	{ MYSQL_TYPE_LONG, N(MYSQL_TYPE_LONG), "INT", T_INTEGER, sizeof( long ), 1 },
	{ MYSQL_TYPE_LONG, N(MYSQL_TYPE_LONG), "INTEGER", T_INTEGER, sizeof( int ), 0 },
	{ MYSQL_TYPE_INT24, N(MYSQL_TYPE_INT24), "MEDIUMINT", T_INTEGER, sizeof( int ) },
	{ MYSQL_TYPE_LONGLONG, N(MYSQL_TYPE_LONGLONG), "BIGINT", T_INTEGER, sizeof( int ) },
	{ MYSQL_TYPE_DOUBLE, N(MYSQL_TYPE_DOUBLE), "DOUBLE", T_DOUBLE, sizeof( double ), 0 },
	{ MYSQL_TYPE_DECIMAL, N(MYSQL_TYPE_DECIMAL), "DECIMAL", T_DOUBLE, sizeof( double ) }, /* or NUMERIC */
	{ MYSQL_TYPE_NEWDECIMAL, N(MYSQL_TYPE_NEWDECIMAL), "NUMERIC", T_DOUBLE, sizeof( double ) }, /* or NUMERIC */
	{ MYSQL_TYPE_FLOAT, N(MYSQL_TYPE_FLOAT), "FLOAT", T_DOUBLE, sizeof( double ), 1 },
	{ MYSQL_TYPE_STRING, N(MYSQL_TYPE_STRING), "CHAR", T_CHAR, 0, 1 },
	{ MYSQL_TYPE_VAR_STRING, N(MYSQL_TYPE_VAR_STRING), "TEXT", T_STRING, 0, 1 },
	{ MYSQL_TYPE_BLOB, N(MYSQL_TYPE_BLOB), "BLOB", T_BINARY, 0, 1 },
	/* This just maps to TINYINT behind the scenes */ 
	{ MYSQL_TYPE_TINY, N(MYSQL_TYPE_TINY), "BOOL", T_BOOLEAN, 0, 1 },
#if 1
	{ MYSQL_TYPE_DATETIME, N(MYSQL_TYPE_DATETIME), "DATETIME", T_DATE, 0, 1 },
	/* If the column is time only, select it */
	{ MYSQL_TYPE_TIMESTAMP, N(MYSQL_TYPE_TIMESTAMP), "DATETIME", T_DATE, 0, 1 },
	/* If the column is time only, select it */
	{ MYSQL_TYPE_TIME, N(MYSQL_TYPE_TIME), "TIME", T_DATE, 0, 1 },
	{ MYSQL_TYPE_DATE, N(MYSQL_TYPE_DATE), "DATE", T_DATE, 0 },
	{ MYSQL_TYPE_YEAR, N(MYSQL_TYPE_YEAR), "YEAR", T_DATE, 0 },
#endif
	{ TYPEMAP_TERM },
};

#endif



// Global variables to ease testing
char delset[ 8 ] = { 13 /*'\r'*/, 10 /*'\n'*/, 0, 0, 0, 0, 0, 0 };
const char ucases[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
const char lcases[] = "abcdefghijklmnopqrstuvwxyz_";
const char exchrs[] = "~`!@#$%^&*()-_+={}[]|:;\"'<>,.?/";

// TODO: Move these into config_t
struct rep { char o, r; } ; //Ghetto replacement scheme...
struct rep **reps = NULL;
int no_unsigned = 0;

// TODO: Specify a delimiter
char *DELIM = ",";

// TODO: Really need to get rid of this completely.
char *no_header = "nothing";

// TODO: This should be set from the command line as well.
case_t case_type = CASE_SNAKE;

#if 0
const char *STDIN = "/dev/stdin";
const char *STDOUT = "/dev/stdout";
char *output_file = NULL;
char *FFILE = NULL;
char *OUTPUT = NULL;
char *INPUT = NULL;
char *prefix = NULL;
char *suffix = NULL;
char *datasource = NULL;
char *table = NULL;
char *stream_chars = NULL;
char *root = "root";
char *streamtype = NULL;
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
int adv = 0;
int show_stats = 0;
int stream_fmt = STREAM_PRINTF;
type_t **gtypes = NULL;
int want_id = 0;
char *id_name = "id";
int want_datestamps = 0;
char *datestamp_name = "date";
int structdata=0;
int classdata=0;
sqltype_t dbengine = SQL_SQLITE3;
char *coercion = NULL;
char *autocast = NULL;
char sqlite_primary_id[] = "id INTEGER PRIMARY KEY AUTOINCREMENT";
const char null_keyword[] = "NULL";
const unsigned long null_keyword_length = sizeof( "NULL" ) - 1;
#endif

/* Might need to do forward declarations... */
#ifdef DEBUG_H
void print_date( date_t * );
void print_dsn ( dsn_t * );
void print_headers( dsn_t * );
void print_stream_type( stream_t );
void print_config( config_t * );
void print_ctypes( coerce_t ** );
#endif

void free_ctypes( coerce_t **ctypes );

/**
 * static const char * get_conn_type( dbtype_t t ) 
 *
 * Get the name of a connection type.
 *
 */
static const char * get_conn_type( dbtype_t t ) {
	return ( t >= 0 && t < sizeof( dbtypes ) / sizeof( char * ) ) ? dbtypes[ t ] : NULL; 
}


/**
 * typemap_t * get_typemap_by_ntype ( const typemap_t *types, int type ) 
 *
 * Get the matching native typemap via the native type integer.
 *
 * NOTE: The common SQL database engines use a number to correspond
 * to a certain type.  Postgres, for instance, uses hard-coded "OIDs"
 * (which are just integers) to point to different types.
 *
 */
typemap_t * get_typemap_by_ntype ( const typemap_t *types, int type ) {
	for ( const typemap_t *t = types; t->ntype != TYPEMAP_TERM; t++ ) {
		if ( t->ntype == type ) return (typemap_t *)t;
	}
	return NULL;
}


/**
 * typemap_t * get_typemap_by_nname ( const typemap_t *types, const char *name ) 
 *
 * Get the matching typemap from the name of a particular type 
 * (like varchar, blob, etc)
 *
 */
typemap_t * get_typemap_by_nname ( const typemap_t *types, const char *name ) {
	for ( const typemap_t *t = types; t->ntype != TYPEMAP_TERM; t++ ) {
		//DPRINTF( "%s: Checking types: %s ?= %s\n", __func__, name, t->name ); 
		if ( !strcasecmp( name, t->name ) ) {
			return (typemap_t *)t;
		}
	}
	return NULL;
}


/**
 * typemap_t * get_typemap_by_btype ( const typemap_t *types, int btype ) 
 *
 * Get the matching typemap from the simplest possible matching type.
 *
 * NOTE: briggs only handles six actual types when looking at data
 * in a spreadsheet or file.  It attempts to map these sensibly to
 * more complicated types in a database engine.
 *
 * See the typemap_t arrays defined above for a practical application
 * of this concept.
 *
 */
typemap_t * get_typemap_by_btype ( const typemap_t *types, int btype ) {
	for ( const typemap_t *t = types; t->ntype != TYPEMAP_TERM; t++ ) {
		//DPRINTF( "%s: Checking types: %s (%s) %d ?= %d\n", __func__, t->name, t->libname, t->basetype, btype );
		//fprintf( stderr, "TYPESTUFF %s %s %d != %d\n", t->typename, t->libtypename, t->basetype, btype );
		if ( btype == t->basetype && t->preferred ) return (typemap_t *)t;
	}
	return NULL;
}


/**
 * static void snakecase ( char **k ) 
 *
 * Convert a word to `snake_case`
 *
 */
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


/**
 * static void camelcase ( char **k ) 
 *
 * Convert a word to `camelCase`.
 *
 */
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


/**
 * static char *dupval ( char *val, char **setval ) 
 *
 * Duplicate a string.
 *
 */
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


/**
 * static char * copy( char *src, int size, int *ns, case_t t ) 
 *
 * Trim and copy a string and use casing rules.
 *
 * TODO: Strictly, this isn't acting on anything that would be 
 * dependent on a char *definition, but this should really be 
 * rewritten to return an unsigned char *.
 *
 */
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


/**
 * static type_t get_type( unsigned char *v, type_t deftype, unsigned int len ) 
 *
 * Checks the  type of value in a field.
 *
 */
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


/**
 * void extract_value_from_column ( char *src, char **dest, int size ) 
 *
 * Get a value from a column
 *
 */
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


/**
 * int check_for_valid_stream ( char *st ) 
 *
 * Checks to see that the user is requesting a valid stream type.
 *
 */
int check_for_valid_stream ( char *st ) {
	for ( streamtype_t *s = streams; s->type; s++ ) {
		if ( !strcmp( s->type, st ) ) {
			return s->streamfmt;
		}
	}
	return -1;
}


/**
 * int safecpypos( const char *src, char *dest, char *delim, int maxlen ) 
 *
 * A function to safely copy a string with an optional delimiter.
 *
 */
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


/**
 * int usafecpynumeric( unsigned char *numtext, long l ) 
 *
 * Copy to a number safely from char *.
 *
 */
int safecpynumeric( char *numtext ) {
	for ( char *p = numtext; *p; p++ ) {
		if ( !memchr( "0123456789", *p, 10 ) ) return -1;
	}

	return atoi( numtext );
}


/**
 * int usafecpynumeric( unsigned char *numtext, long l ) 
 *
 * Copy to a number safely from unsigned char *.
 *
 */
int usafecpynumeric( unsigned char *numtext, long l ) {
	long len = l;
	char *jp = NULL;
	int num = 0;

	// Check for the stupid thing
	for ( unsigned char *p = numtext; len && *p; p++, len-- )
		if ( !memchr( "0123456789", *p, 10 ) ) return -1;

	// Copy into a buffer to use atoi
	if ( !( jp = malloc( l + 1 ) ) || !memset( jp, 0, l ) ) {
		return -1;
	}

	// Copy from original block
	memcpy( jp, numtext, l );	
	num = atoi( jp );
	free( jp );
	return num;
}


/**
 * int parse_dsn_info( dsn_t *conn, char *err, int errlen ) 
 *
 * Parse data source string into a dsn_t.
 *
 * NOTE: We only support URI styles for consistency.
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



/**
 * void destroy_dsn_headers ( dsn_t *t ) 
 *
 * Free all header data allocated when interacting with a data source.
 *
 */
void destroy_dsn_headers ( dsn_t *t ) {
	for ( header_t **s = t->headers; s && *s; s++ ) {
		free( *s );
	}
	free( t->headers );
}


/**
 * void destroy_dsn_rows ( dsn_t *t ) 
 *
 * Free all rows allocated when interacting with a data source.
 *
 */
void destroy_dsn_rows ( dsn_t *t ) {
	WHERE();
	for ( row_t **r = t->rows; r && *r; r++ ) {
		for ( column_t **c = (*r)->columns; c && *c; c++ ) free( (*c) );
		free( (*r)->columns );
		free( (*r) );
	}
	free( t->rows );
}


/**
 * int schema_from_dsn( dsn_t *iconn, dsn_t *oconn, char *fm, int fmtlen, char *err, int errlen ) 
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
	#if 0
		// Getting the type needs to be a bit smarter now
		typemap_t *type = NULL;

		if ( (*s)->ctype ) {
			type = (*s)->ctype;
		}
		// TODO: In the case of TIME only, the default DATETIME selection won't work
		else if ( !( type = get_typemap_by_btype( oconn->typemap, (*s)->type ) ) ) {
			const char fmt[] = "No available type found for column '%s' %s";
			snprintf( err, errlen, fmt, (*s)->label, itypes[ (*s)->type ] );
			return 0;
		}
		/* map i->db type to o->db type here */
	#endif

		//const char fmt[] = "\t%s %s";
		// TODO: To save some space, I'd be happy with get_ptypename( ... );
		written = snprintf( fmt, fmtlen, "\t%s %s", (*s)->label, (*s)->ptype->name );
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
 * int test_dsn (dsn_t *conn, char *err, int errlen) 
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
FPRINTF( "SQlite output conn test failed.\n" );
		return 0;
	}
	else if ( conn->type == DB_MYSQL ) {
		// You'll need to be able to connect and test for existence of db and table
FPRINTF( "MySQL output conn test failed.\n" );
		return 0;
	}
	else if ( conn->type == DB_POSTGRESQL ) {
		// You'll need to be able to connect and test for existence of db and/or table
FPRINTF( "Postgres output conn test failed.\n" );
		return 0;
	}
	return 1;
}


/**
 * int create_dsn ( dsn_t *, dsn_t *, char *, int )
 *
 * Create a data source.  (A file, database if SQLite3 or
 * a table if a super heavy weight database.)
 *
 */
int create_dsn ( dsn_t *ic, dsn_t *oc, char *err, int errlen ) {
	// This might be a little harder, unless there is a way to either create a new connection
	char schema_fmt[ MAX_STMT_SIZE ];
	memset( schema_fmt, 0, MAX_STMT_SIZE );
#if 0
	const char create_database_stmt[] = "CREATE DATABASE IF NOT EXISTS %s";
#endif

	// Simply create the file.  Do not open it
	if ( oc->type == DB_FILE ) {
		const int flags =	O_RDWR | O_CREAT | O_TRUNC;
		const mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
		if ( open( oc->connstr, flags, perms ) == -1 ) {
			snprintf( err, errlen, "%s", strerror( errno ) );
			return 0;
		}
		return 1;
	}

	// Every other engine will need a schema
	if ( !schema_from_dsn( ic, oc, schema_fmt, sizeof( schema_fmt ), err, errlen ) ) {
		return 0;
	}

#if 0
	// TODO: Some engines can't run with a semicolon
	for ( char *s = schema_fmt; *s; s++ ) {
		// TODO: Try `if ( *s == ';' && *s = '\0' ) break`
		if ( *s == ';' ) {
			*s = '\0';	
			break;
		}
	}

fprintf( stdout, "Generated schema: %s\n", schema_fmt );
#endif

	// SQLite: create table (db ought to be created when you open it)
	if ( oc->type == DB_SQLITE ) {

	}
#ifdef BMYSQL_H
	// MySQL: create db & table (use an if not exists?, etc)
	else if ( oc->type == DB_MYSQL ) {
FPRINTF( "Attempting to create new MySQL table from %s.\n", schema_fmt );
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
		if ( !strlen( oc->dbname ) ) {
			snprintf( err, errlen, "Database name was not specified for output datasource" );
			return 0;
		}

		// Then try to create the db
		t = mysql_real_connect(
			myconn,
			oc->hostname,
			strlen( oc->username ) ? oc->username : NULL,
			strlen( oc->password ) ? oc->password : NULL,
			oc->dbname,
			oc->port,
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
FPRINTF( "Creation of MySQL table succeeded\n" );
	}
#endif


#ifdef BPGSQL_H
	// Postgres: create db & table (use an if not exists?, etc)
	else if ( oc->type == DB_POSTGRESQL ) {
FPRINTF( "Attempting to create new PostgreSQL table from %s.\n", schema_fmt );
		// Try to connect to insetance first
		PGconn *pgconn = NULL;
		PGresult *pgres = NULL;
		char *cs = NULL;
		char cs_buffer[ MAX_STMT_SIZE ];
		memset( cs_buffer, 0, MAX_STMT_SIZE );

		// Handle cases in which the database does not exist
		if ( strlen( oc->dbname ) ) {
			cs = oc->connstr;
		}
		else {
			const char defname[] = "postgres";

			// This is going to prove difficult too.
			// Since Postgres forces us to choose a database
			if ( !strlen( oc->username ) && !strlen( oc->password ) )
				snprintf( cs_buffer, sizeof( cs_buffer ), "host=%s port=%d dbname=%s", oc->hostname, oc->port, defname );
			else if ( !strlen( oc->password ) )
				snprintf( cs_buffer, sizeof( cs_buffer ), "host=%s port=%d user=%s dbname=%s", oc->hostname, oc->port, oc->username, defname );
			else {
				const char fmt[] = "host=%s port=%d user=%s password=%s dbname=%s";
				snprintf( cs_buffer, sizeof( cs_buffer ), fmt, oc->hostname, oc->port, oc->username, oc->password, defname );
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

		// NOTE: 2024-02-15: 
		// Per PQexec docs, pgres MAY very well be NULL after this.  Check it
		// anyway and use PQerrorMessage(...) to get more info
		pgres = PQexec( pgconn, schema_fmt );

		if ( PQresultStatus( pgres ) != PGRES_COMMAND_OK ) {
			const char fmt[] = "Failed to run query against selected db and table: '%s'";
			snprintf( err, errlen, fmt, PQerrorMessage( pgconn ) );
			PQfinish( pgconn );
			return 0;
		}

		// Need to clear and preferably destroy it...
		PQclear( pgres );
		PQfinish( pgconn );
FPRINTF( "Creation of PostgreSQL table succeeded\n" );
	}
#endif

	return 1;
}



#if 0
/**
 * int modify_pgconnstr ( char *connstr, int fmtlen ) 
 *
 * Modify the Postgres connection string without making a duplicate.
 *
 */
static int modify_pgconnstr ( char *connstr, int fmtlen ) {
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
#endif


/**
 * int open_dsn ( dsn_t *conn, config_t *conf, const char *qopt, char *err, int errlen ) 
 *
 * Open a manage a data source.
 *
 */
int open_dsn ( dsn_t *conn, config_t *conf, char *err, int errlen ) {
	WPRINTF( "Attempting to open connection indicated by '%s'", conn->connstr );
	char query[ 2048 ];
	memset( query, 0, sizeof( query ) );

	// Handle opening the database first
	if ( conn->type == DB_FILE ) {
		// An mmap() implementation for speedy reading and less refactoring
		struct stat sb;
		file_t *file = NULL;

		// Create a file structure
		if ( !( file = malloc( sizeof( file_t ) ) ) || !memset( file, 0, sizeof( file_t ) ) ) {
			snprintf( err, errlen, "malloc() %s", strerror( errno ) );
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
			snprintf( err, errlen, "stat() %s", strerror( errno ) );
			free( file );
			return 0;
		}

		// Open the file
		if ( ( file->fd = open( conn->connstr, O_RDONLY ) ) == -1 ) {
			snprintf( err, errlen, "open() %s", strerror( errno ) );
			free( file );
			return 0;
		}

		// Do stuff
		file->size = sb.st_size;
		file->map = mmap( 0, file->size, PROT_READ, MAP_PRIVATE, file->fd, 0 );
		if ( file->map == MAP_FAILED ) {
			snprintf( err, errlen, "mmap() %s", strerror( errno ) );
			free( file );
			return 0;
		}

		file->start = file->map;
		file->offset = 0;
		conn->conn = file;
		return 1;
	}

	// Stop if no table name was specified
	if ( !conf->wtable && ( !( *conn->tablename ) || strlen( conn->tablename ) < 1 ) ) {
		snprintf( err, errlen, "No table name specified for source data." );
		return 0;
	}

	// Create the final query from here too
	if ( conf->wquery )
		snprintf( query, sizeof( query ) - 1, "%s", conf->wquery );
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


/**
 * static const char * find_ctype ( coerce_t **list, const char *name ) 
 *
 * Locate a specific coerced type.
 *
 */
static const char * find_ctype ( coerce_t **list, const char *name ) {
	for ( coerce_t **x = list; x && *x; x++ ) {
		if ( !strcasecmp( (*x)->misc, name ) ) return (*x)->typename;
	}
	return NULL;
}


/**
 * static const char * find_atype ( coerce_t **list, const char *name ) 
 *
 * Locate a specific coerced type.
 *
 */
static const char * find_atype ( coerce_t **list, const char *typename ) {
	for ( coerce_t **x = list; x && *x; x++ ) {
		if ( !strcasecmp( (*x)->misc, typename ) ) return (*x)->typename;
	}
	return NULL;
}



/**
 * coerce_t * create_ctypes( const char *src, char *err, int errlen ) 
 *
 * Create a key-value list of column names and their "coerced" type matches.
 *
 */
coerce_t ** create_ctypes( const char *src, dsn_t *iconn, dsn_t *oconn, char *err, int errlen ) {

	// Define stuff
	coerce_t **ctypes = NULL;
	int ctypeslen = 0;
	zw_t pp = { 0 };
	const char *fmt = ( !iconn ) ? "--coerce" : "--auto";

	// No matter who is calling, `oconn` is always needed
	if ( !oconn ) {
		fprintf( stderr, "NO OUPTUT CONNECTION GIVEN TO %s", __func__ );
		return NULL;
	}

	// Loop through however many may exist
	for ( coerce_t *t = NULL; strwalk( &pp, src, "," ); t = NULL ) {
		if ( pp.size - 1 ) {
			zw_t px = { 0 };
			unsigned char *n = NULL; 
			char p[ 256 ] = { 0 };
			int len = pp.size - 1;

			// Copy to new buffer or fail
			if ( ( pp.size - 1 ) < sizeof( p ) ) {
				n = trim( (unsigned char *)&src[ pp.pos ], " ", pp.size - 1, &len );
				memcpy( p, n, len );
			}
			else {
				memcpy( p, &src[ pp.pos ], 32 );
				memcpy( &p[ 32 ], "...", 3 );
				snprintf( err, errlen, "Argument given to %s will truncate: '%s'", fmt, p );
				return NULL;
			}
		
			// Stop if there are too many
			if ( memchrocc( p, '=', strlen( p ) ) != 1 ) {
				snprintf( err, errlen, "Argument given to %s is misformatted: '%s'", fmt, p );
				return NULL;
			}

			// Catch allocation failures
			if ( !( t = malloc( sizeof( coerce_t ) ) ) || !memset( t, 0, sizeof( coerce_t ) ) ) {
				snprintf( err, errlen, "Allocation failed(): %s\n", strerror( errno ) );
				return NULL;
			}

			// Then break the type and thing 
			for ( int i = 0; strwalk( &px, p, "=" ); i++ ) {
				// Define more
				int len = 0;
				char b[ 128 ] = { 0 };
				unsigned char *n = NULL;
				typemap_t *tmap = NULL;

				// Check for zero-length or overflow and copy to buffer
				if ( ( px.size - 1 ) < 1 ) {
					// This is a format error
					snprintf( err, errlen, "Got zero-length argument at %s", fmt );
					return NULL;
				}
				else if ( ( px.size - 1 ) > sizeof( b ) ) {
					snprintf( err, errlen, "Argument given to %s will truncate: '%s'", fmt, p );
					return NULL;
				}
				else {
					n = trim( (unsigned char *)&p[ px.pos ], " ", px.size - 1, &len );
					memcpy( b, n, len );
				}	

				// Get the key
				if ( i )
					snprintf( (char *)t->typename, sizeof( t->typename ), "%s", b );
				else {
					// No input means that this first key is a column label
					if ( !iconn )
						snprintf( (char *)t->misc, sizeof( t->misc ), "%s", b );
					else {
						// Check that the input type is even supported
						if ( !( tmap = get_typemap_by_nname( iconn->typemap, b ) ) ) {
							free( t );
							snprintf( err, errlen, "Type '%s' not found in argument at %s", b, fmt );
							return NULL;	
						}

						snprintf( (char *)t->misc, sizeof( t->misc ), "%s", tmap->name );
					}
				}
			}

			// Add this to the list...
			add_item( &ctypes, t, coerce_t *, &ctypeslen );
		}
	}

	return ctypes;
}


/**
 * headers_from_dsn( dsn_t * )
 *
 * Print the headers from a data source.
 *
 */
int headers_from_dsn ( dsn_t *conn, config_t *conf, char *err, int errlen ) {

	// Empty condition to assist with compiling out code
	if ( conn->type == DB_FILE ) {
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
		for ( int len = 0; strwalk( &p, (char *)file->map, delset ); ) {
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

		// TODO: You CAN still make a schema with an empty query...
		// But this is kind of tricky to allow.
		if ( !conn->hlen ) {
			const char fmt[] = "No header data found, please double check the file or input query.";  
			snprintf( err, errlen, fmt );
			return 0;
		}
		else if ( conn->hlen < 2 ) {
			const char fmt[] = "Array size is less than 2.  Does this look correct?: %s";  
			header_t **header = conn->headers;
			snprintf( err, errlen, fmt, (*header)->label );
			return 0;
		}
	}
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

		// TODO: You CAN still make a schema with an empty query...
		// But this is kind of tricky to allow.
		if ( conn->hlen < 1 ) {
			const char fmt[] = "No data was returned in the query.  Not attempting to generate headers.";
			snprintf( err, errlen, fmt );
			return 0;
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
#ifdef BSQLITE_H
	else if ( conn->type == DB_SQLITE ) {
		snprintf( err, errlen, "%s", "SQLite3 driver not done yet" );
		return 0;
	}
#endif


	return 1;
}



#if 0
// TODO: This is coming back, just not right now...
/**
 * struct_from_dsn( dsn_t * )
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
#endif


/**
 * char * create_sql_insert_statement ( dsn_t *oconn, dsn_t *iconn, bindtype_t bindtype, char *err, int errlen ) 
 *
 * Create an SQL insert statement suitable for a specific type of database engine.
 *
 */
char * create_sql_insert_statement ( dsn_t *oconn, dsn_t *iconn, bindtype_t bindtype, char *err, int errlen ) {
	int p = 0;
	char *bindstmt = NULL;
	char  buffer[ 512 ] = { 0 };
	const char *fmt = NULL;

	// TODO: Complians about not const string, fix this...
	if ( bindtype == BINDTYPE_ANON )
		fmt = bindtypes[ BINDTYPE_ANON ];
	else if ( bindtype == BINDTYPE_NUMERIC )
		fmt = bindtypes[ BINDTYPE_NUMERIC ];
	else if ( bindtype == BINDTYPE_ALPHA )
		fmt = bindtypes[ BINDTYPE_ALPHA ];
	else if ( bindtype == BINDTYPE_AT ) {
		fmt = bindtypes[ BINDTYPE_AT ];
	}

	// Also go ahead and create the reusable bind statement
	if ( !( bindstmt = malloc( 1 ) ) || !memset( bindstmt, 0, 1 ) ) {
		const char fmt[] = "%s: out of memory occurred allocating MySQL bind statement: %s";
		snprintf( err, errlen, fmt, __func__, strerror( errno ) );
		return 0;	
	}

	// Write the "preamble"
	p = snprintf( buffer, sizeof( buffer ), "INSERT INTO %s ( ", oconn->tablename ); 
	if ( p >= sizeof( buffer ) - 1 || buffer[p] != '\0' ) {
		const char fmt[] = "%s: Truncation occurred writing MySQL bind statement";
		snprintf( err, errlen, fmt, __func__ );
		return 0;
	}

	// Copy it and then loop through the rest
	bindstmt = realloc( bindstmt, p );
	memcpy( bindstmt, buffer, p );

	// All column headers 
	for ( header_t **h = iconn->headers; h && *h; h++ ) {
		int i = p;
		int size = 0;
		memset( buffer, 0, sizeof( buffer ) );
		size = snprintf( buffer, sizeof( buffer ) - 1 , &",%s"[ ( *iconn->headers == *h ) ], (*h)->label );
		p += size;
		bindstmt = realloc( bindstmt, p );
		memcpy( &bindstmt[ i ], buffer, size );	
	}

	// Then VALUES string
	if ( 1 ) {
		int i = p;
		int size = 0;
		memset( buffer, 0, sizeof( buffer ) );
		size = snprintf( buffer, sizeof( buffer ), " ) VALUES ( " );
		p += size;
		bindstmt = realloc( bindstmt, p );
		memcpy( &bindstmt[ i ], buffer, size );
	}

	// Then each parameter 
	// All column headers 
	int column = 1;
	for ( header_t **h = iconn->headers; h && *h; h++, column++ ) {
		int i = p;
		int size = 0;
		int f = ( *iconn->headers == *h ) ? 0 : 1;
		memset( buffer, 0, sizeof( buffer ) );

		if ( f ) {
			size = snprintf( buffer, sizeof( buffer ), "," );
		} 

		if ( bindtype == BINDTYPE_ANON )
			size += snprintf( &buffer[f], sizeof( buffer ) - 1, "%s", fmt );
		else if ( bindtype == BINDTYPE_NUMERIC )
			size += snprintf( &buffer[f], sizeof( buffer ) - 1 , fmt, column ); 
		else if ( bindtype == BINDTYPE_ALPHA )
			size += snprintf( &buffer[f], sizeof( buffer ) - 1 , fmt, (*h)->label );
		else if ( bindtype == BINDTYPE_AT ) {
			size += snprintf( &buffer[f], sizeof( buffer ) - 1 , fmt, (*h)->label ); 
		}

		p += size;
		bindstmt = realloc( bindstmt, p );
		memcpy( &bindstmt[ i ], buffer, size );	
	}
	
	// Then the end	
	if ( 1 ) {
		int i = p;
		int size = 0;
		memset( buffer, 0, sizeof( buffer ) );
		size = snprintf( buffer, sizeof( buffer ), " )" );
		p += size;
		bindstmt = realloc( bindstmt, p );
		memcpy( &bindstmt[ i ], buffer, size );
	}

	// Terminate 
	bindstmt = realloc( bindstmt, p + 1 );
	memcpy( &bindstmt[ p ], "\0", 1 );
	return bindstmt;
}


/**
 * int prepare_dsn_for_read ( dsn_t *conn, char *err, int errlen ) 
 *
 * Prepare DSN for reading.  If we processed a limit, that would
 * really be the only way to make this work sensibly with SQL.
 *
 */
int prepare_dsn_for_read ( dsn_t *conn, char *err, int errlen ) {
	// Move up the buffer so that we start from the right offset
	//while ( *(buffer++) != '\n' );
	WHERE();

	if ( conn->type == DB_FILE ) {
		file_t *file = (file_t *)conn->conn;
		int len = file->size;

		// Move to the end of the first row
		while ( len-- && *(file->map)++ != '\n' ) {
			//file->map++, len--;
		}

		file->offset = file->size - len;
		DPRINTF( "file->offset: %d\n", file->offset );
		DPRINTF( "file->size: %ld\n", file->size );
		DPRINTF( "file->map: %p\n", (void *)file->map );
	}

	else if ( conn->type == DB_MYSQL ) {
	}

	else if ( conn->type == DB_POSTGRESQL ) {
	}

	return 1;
}


/**
 * int prepare_dsn_for_write ( dsn_t *, dsn_t *, char *, int) 
 *
 * Prepare DSN for writing.  
 *
 */
int prepare_dsn_for_write ( dsn_t *oconn, dsn_t *iconn, char *err, int errlen ) {
	WHERE();

	if ( oconn->type == DB_FILE ) {
		file_t *file = (file_t *)oconn->conn;
		// Reading happens from the input source, so that is another story
		if ( oconn->stream == STREAM_PRINTF || oconn->stream == STREAM_COMMA ) 
			;
		if ( oconn->stream == STREAM_CSTRUCT )
			;
		if ( oconn->stream == STREAM_CARRAY )
			;
		if ( oconn->stream == STREAM_JSON )
			FDPRINTF( file->fd, "[" );
		if ( oconn->stream == STREAM_XML ) {
			char *treename = strlen( oconn->dbname ) ? oconn->dbname : "tree";
			FDPRINTF( file->fd, "<" );
			FDPRINTF( file->fd, treename );
			FDPRINTF( file->fd, ">" );
			FF.newline ? FDPRINTF( file->fd, "\n" ) : 0;
		}	
	}


	else if ( oconn->type == DB_MYSQL ) {
		WPRINTF( "PREPARING MySQL DSN: %d columns\n", iconn->hlen );

		// Deref and check that this is still here...
		mysql_t *t = (mysql_t *)oconn->conn;	
	
		// Make the insert statement
		if ( !( t->query = create_sql_insert_statement( oconn, iconn, BINDTYPE_ANON, err, errlen ) ) ) {
			return 0;
		}

		// Allocate statement and bind arguments structures
		t->stmt = (void *)mysql_stmt_init( (MYSQL *)t->conn );

		if ( !( t->bindargs = malloc( sizeof( MYSQL_BIND ) * iconn->hlen ) ) || !memset( t->bindargs, 0, sizeof( MYSQL_BIND ) * iconn->hlen ) ) {
			const char fmt[] = "%s: out of memory occurred allocating MySQL bind structures: %s";
			snprintf( err, errlen, fmt, __func__, strerror( errno ) );
			return 0;	
		}
	}

	else if ( oconn->type == DB_POSTGRESQL ) {
		WPRINTF( "PREPARING PostgreSQL DSN: %d columns\n", iconn->hlen );

		// Der
		pgsql_t *t = (pgsql_t *)oconn->conn;	

		// Make the insert statement
		if ( !( t->query = create_sql_insert_statement( oconn, iconn, BINDTYPE_NUMERIC, err, errlen ) ) ) {
			return 0;
		}

		if ( !( t->bindargs = malloc( sizeof( char * ) * iconn->hlen ) ) || !memset( t->bindargs, 0, sizeof( char * ) * iconn->hlen ) ) {
			const char fmt[] = "%s: out of memory occurred allocating Postgres bind structures: %s";
			snprintf( err, errlen, fmt, __func__, strerror( errno ) );
			return 0;
		}

		if ( !( t->bindlens = malloc( sizeof( int ) * iconn->hlen ) ) || !memset( t->bindargs, 0, sizeof( int ) * iconn->hlen ) ) {
			const char fmt[] = "%s: out of memory occurred allocating Postgres bind length structures: %s";
			snprintf( err, errlen, fmt, __func__, strerror( errno ) );
			return 0;	
		}

		if ( !( t->bindoids = malloc( sizeof( char * ) * iconn->hlen ) ) || !memset( t->bindoids, 0, sizeof( char * ) * iconn->hlen ) ) {
			const char fmt[] = "%s: out of memory occurred allocating Postgres bind types structures: %s";
			snprintf( err, errlen, fmt, __func__, strerror( errno ) );
			return 0;
		}

		if ( !( t->bindfmts = malloc( sizeof( int ) * iconn->hlen ) ) || !memset( t->bindfmts, 0, sizeof( int ) * iconn->hlen ) ) {
			const char fmt[] = "%s: out of memory occurred allocating Postgres bind types structures: %s";
			snprintf( err, errlen, fmt, __func__, strerror( errno ) );
			return 0;
		}
	}

	return 1;
}


/**
 * void unprepare_dsn( dsn_t *conn ) 
 *
 * "Unmount" different data sources (e.g. closing a database or something)
 *
 */
void unprepare_dsn( dsn_t *conn ) {
	// Rewinding the pointer to the beginning would be an ok idea. But unnecessary
	if ( conn->type == DB_FILE ) {
		WPRINTF( "UNPREPARING FILE DSN\n" );
		file_t *file = (file_t *)conn->conn;
		// Reading happens from the input source, so that is another story
		if ( conn->stream == STREAM_PRINTF || conn->stream == STREAM_COMMA ) 
			;
		else if ( conn->stream == STREAM_CSTRUCT )
			;
		else if ( conn->stream == STREAM_CARRAY )
			;
		else if ( conn->stream == STREAM_JSON )
			FDPRINTF( file->fd, "]" );
		else if ( conn->stream == STREAM_XML ) {
			char *treename = strlen( conn->dbname ) ? conn->dbname : "tree";
			FDPRINTF( file->fd, "</" );
			FDPRINTF( file->fd, treename );
			FDPRINTF( file->fd, ">" );
		}
		
	}

	else if ( conn->type == DB_MYSQL ) {
		WPRINTF( "UNPREPARING MySQL DSN\n" );
		mysql_t *t = (mysql_t *)conn->conn;	
		mysql_stmt_close( t->stmt );
		free( t->bindargs );
		free( (void *)t->query );
	}

	else if ( conn->type == DB_POSTGRESQL ) {
		WPRINTF( "UNPREPARING Postgres DSN\n" );
		pgsql_t *t = (pgsql_t *)conn->conn;	
		free( t->bindargs );
		free( t->bindlens );
		free( t->bindfmts );
		free( t->bindoids );
		free( (void *)t->query );
	}
}



/**
 * records_from_dsn( dsn_t * )
 *
 * Extract records from a data source.
 *
 * conn = The source we're reading records from
 * count = Number of "rows" to read at a time
 * offset = Where to start from next
 *
 */
int records_from_dsn( dsn_t *conn, int count, int offset, char *err, int errlen ) {
	WHERE();

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
					const char fmt[] = "Type check for value at row %d, column '%s' (%d) failed. (Expected %s, got %s)";
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
#ifdef BMYSQL_H
	else if ( conn->type == DB_MYSQL ) {
		// Cast
		mysql_t *b = (mysql_t *)conn->conn;
		unsigned int rowcount = 0;

		if ( !b ) {
			const char fmt[] = "mysql() connection lost";
			snprintf( err, errlen, fmt );
			return 0;
		}

		// Catch this unusual situation (should probably be an error)
		if ( !( rowcount = mysql_num_rows( b->res ) ) ) {
			snprintf( err, errlen, "Query returned no rows." );
			return 0;	
		}

		// Move through all of the columns
		for ( unsigned int i = 0; i < rowcount ; i++ ) {
			MYSQL_ROW r = mysql_fetch_row( b->res );
			row_t *row = NULL;
			column_t **cols = NULL;
			unsigned long *lens = mysql_fetch_lengths( b->res );

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
				col->v = (unsigned char *)( *r );
				col->len = lens[ ci ];
				// This only gets the basetype
				col->type = conn->headers[ ci ]->type;
#if 0
				// If it's a date, then let's just see what's in the engine for now
				if ( col->type == T_DATE ) {
					date_t *d = &col->date;
					memset( d, 0, sizeof( date_t ) );
					unsigned char *v = col->v;
					typemap_t *x = conn->headers[ ci ]->ntype;

					if ( x->ntype == MYSQL_TYPE_YEAR ) {
						d->year = usafecpynumeric( v, 4 );  
					}
					else if ( x->ntype == MYSQL_TYPE_DATE ) {
						d->year = usafecpynumeric( v, 4 ), v += 5;
						d->month = usafecpynumeric( v, 2 ), v += 3;
						d->day = usafecpynumeric( v, 2 );
					}
					else if ( x->ntype == MYSQL_TYPE_TIME ) {
						// Check if the hour is 3 lengths
						d->hour = usafecpynumeric( v, 2 ), v += 3;
						d->minute = usafecpynumeric( v, 2 ), v += 3;
						d->second = usafecpynumeric( v, 2 );
					}
					else if ( x->ntype == MYSQL_TYPE_TIMESTAMP || x->ntype == MYSQL_TYPE_DATETIME ) {
						d->year = usafecpynumeric( v, 4 ), v += 5;
						d->month = usafecpynumeric( v, 2 ), v += 3;
						d->day = usafecpynumeric( v, 2 ), v += 3;
						// Check if the hour is 3 lengths
						d->hour = usafecpynumeric( v, 2 ), v += 3;
						d->minute = usafecpynumeric( v, 2 ), v += 3;
						d->second = usafecpynumeric( v, 2 );
					}
				}
#endif
				// Add to the column set
				add_item( &cols, col, column_t *, &clen );
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
			add_item( &conn->rows, row, row_t *, &conn->rlen );
			cols = NULL;
		}
	}
#endif
#ifdef BPGSQL_H
 #if 1
	else if ( conn->type == DB_POSTGRESQL )  {
		pgsql_t *b = (pgsql_t *)conn->conn;
		int rcount = PQntuples( b->res );
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
				col->len = PQgetlength( b->res, i, ci );
				col->type = conn->headers[ ci ]->type;
			#if 1
				col->v = (unsigned char *)PQgetvalue( b->res, i, ci );
			#else
				// If the network connection cuts out, it's very possible that this data will be gone
				//col->v = (unsigned char *)strdup( PQgetvalue( conn->res, i, ci ) );
			#endif

#if 0
				// If it's a date, then let's just see what's in the engine for now
				if ( col->type == T_DATE ) {
					date_t *d = &col->date;
					memset( d, 0, sizeof( date_t ) );
					unsigned char *v = col->v;
					typemap_t *x = conn->headers[ ci ]->ntype;

//DPRINTF( "Field %-10s is a date (type %d %s)!\n", col->k, x->ntype, x->libtypename );
//write( 2, "'", 1 ), write( 2, col->v, col->len ), write( 2, "'\n", 2 );
					if ( x->ntype == PG_DATEOID ) {
						d->year = usafecpynumeric( v, 4 ), v += 5;
						d->month = usafecpynumeric( v, 2 ), v += 3;
						d->day = usafecpynumeric( v, 2 );
					}
					else if ( x->ntype == PG_TIMEOID ) {
						// Check if the hour is 3 lengths
						d->hour = usafecpynumeric( v, 2 ), v += 3;
						d->minute = usafecpynumeric( v, 2 ), v += 3;
						d->second = usafecpynumeric( v, 2 );
					}
					else if ( x->ntype == PG_TIMESTAMPOID ) {
						d->year = usafecpynumeric( v, 4 ), v += 5;
						d->month = usafecpynumeric( v, 2 ), v += 3;
						d->day = usafecpynumeric( v, 2 ), v += 3;
						// Check if the hour is 3 lengths
						d->hour = usafecpynumeric( v, 2 ), v += 3;
						d->minute = usafecpynumeric( v, 2 ), v += 3;
						d->second = usafecpynumeric( v, 2 );
					}
				}
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
 *
 * "Transform" the records into whatever output format you're looking for.
 *
 */
int transform_from_dsn(
	dsn_t *iconn,
	dsn_t *oconn,
	int count,
	int offset,
	char *err,
	int errlen
) {

	// Define
	WHERE();
	char insfmt[ 256 ];
	memset( insfmt, 0, sizeof( insfmt ) );

	// Define any db specific stuff
	int ri = 0;

	// Loop through all rows
	for ( row_t **row = iconn->rows; row && *row; row++, ri++ ) {
		char ffmt[ MAX_STMT_SIZE ];
		int first = *row == *iconn->rows;
		int ci = 0;

		// memset
		memset( ffmt, 0, MAX_STMT_SIZE );

		//Prefix
		if ( oconn->type == DB_FILE ) {
			file_t *file = (file_t *)oconn->conn;
			( FF.prefix ) ? FDPRINTF( file->fd, FF.prefix ) : 0;

			if ( oconn->stream == STREAM_PRINTF || oconn->stream == STREAM_COMMA ) 
				;
			else if ( oconn->stream == STREAM_CSTRUCT )
				FDPRINTF( file->fd, &",{"[!( offset || !first )] );
			else if ( oconn->stream == STREAM_CARRAY )
				FDPRINTF( file->fd, &",{"[!( offset || !first )] );
			else if ( oconn->stream == STREAM_JSON ) {
				FDPRINTF( file->fd, &",{"[!( offset || !first )] );
				( FF.newline ) ? FDPRINTF( file->fd, "\n" ) : 0;
			}
			else if ( oconn->stream == STREAM_XML ) {
				FF.pretty ? FDPRINTF( file->fd, "\t" ) : 0;
				FDPRINTF( file->fd, "<" );
				// TODO: Should this be auto-inferred?
				FDPRINTF( file->fd, oconn->tablename );
				FDPRINTF( file->fd, ">" );
				( FF.newline ) ? FDPRINTF( file->fd, "\n" ) : 0;
				//( FF.newline ) ? FDPRINTF( file->fd, "\n" ) : 0;
			}
			else if ( oconn->stream == STREAM_SQL ) {
			#if 0
				// Way faster to use what you've already written..., but you can't use the whole thing...
				// You can prepare it if stream_SQL is chosen...
			#else
				FDPRINTF( file->fd, "INSERT INTO " );
				FDPRINTF( file->fd, oconn->tablename );
				FDPRINTF( file->fd, " (" );
				for ( header_t **x = iconn->headers; x && *x; x++ ) {
					( *iconn->headers != *x ) ? FDPRINTF( file->fd, "," ) : 0;
					FDPRINTF( file->fd, (*x)->label );
				}
				FDPRINTF( file->fd, ") VALUES (" );
			}
			#endif
		}

		// Loop through each column
		for ( column_t **col = (*row)->columns; col && *col; col++, ci++ ) {
			int first = *col == *((*row)->columns);
			if ( oconn->stream == STREAM_PRINTF ) {
				file_t *file = (file_t *)oconn->conn;
				FDPRINTF( file->fd, (*col)->k );
				FDPRINTF( file->fd, " => " );
				if ( (*col)->type == T_BOOLEAN && memchr( "Tt1", *(*col)->v, 3 ) )
					FDPRINTF( file->fd, "true" );
				else if ( (*col)->type == T_BOOLEAN && memchr( "Ff0", *(*col)->v, 3 ) )
					FDPRINTF( file->fd, "false" );
				else if ( (*col)->type == T_BINARY ) {
					char bin[ 64 ];
					memset( &bin, 0, sizeof( bin ) );
					snprintf( bin, sizeof( bin ) - 1, "%p (%ld bytes)", (void *)(*col)->v, (*col)->len );
					FDPRINTF( file->fd, bin );
				}
				else if ( (*col)->type == T_STRING || (*col)->type == T_CHAR ) {
					FDPRINTF( file->fd, FF.leftdelim );
					FDNPRINTF( file->fd, (*col)->v, (*col)->len );
					FDPRINTF( file->fd, FF.rightdelim );
				}
			#if 0
				else if ( (*col)->type == T_DATE ) {
					// Try a custom date format?
				}
			#endif
				else {
					FDNPRINTF( file->fd, (*col)->v, (*col)->len );
				}
				FDPRINTF( file->fd, "\n" );
			}
			else if ( oconn->stream == STREAM_RAW ) {
				file_t *file = (file_t *)oconn->conn;
				( first ) ? 0 : FDPRINTF( file->fd, oconn->cold );
				FDNPRINTF( file->fd, (*col)->v, (*col)->len );
			}
			else if ( oconn->stream == STREAM_XML ) {
				file_t *file = (file_t *)oconn->conn;
				unsigned char *v = (*col)->v;
				unsigned long len = (*col)->len;
				( FF.pretty ) ? FDPRINTF( file->fd, "\t\t" ) : 0;
				FDPRINTF( file->fd, "<" );
				FDPRINTF( file->fd, (*col)->k );
				FDPRINTF( file->fd, ">" );
				//TODO: Like JSON quote substitution, this could be incredibly slow
				//TODO: We'll just be blank if there are no values, but do we want this?
				for ( unsigned char *c = NULL; len; len--, v++ ) {
					if ( !( c = memchr( "<>&\"'", *v, 5 ) ) )
						FDNPRINTF( file->fd, v, 1 );
					else if ( *c == '<' )
						FDPRINTF( file->fd, "&lt;" );
					else if ( *c == '>' )
						FDPRINTF( file->fd, "&gt;" );
					else if ( *c == '&' )
						FDPRINTF( file->fd, "&amp;" );
					else if ( *c == '"' )
						FDPRINTF( file->fd, "&quot;" );
					else if ( *c == '\'' ) {
						FDPRINTF( file->fd, "&apos;" );
					}	
				}
				FDPRINTF( file->fd, "</" );
				FDPRINTF( file->fd, (*col)->k );
				//FDNPRINTF( file->fd, ">\n", ( FF.newline ) ? 2 : 1 );
				FDPRINTF( file->fd, ">" );
				( FF.newline ) ? FDPRINTF( file->fd, "\n" ) : 0;
			}
			else if ( oconn->stream == STREAM_CARRAY ) {
				file_t *file = (file_t *)oconn->conn;
				FDPRINTF( file->fd, &", \""[ first ] );
				FDNPRINTF( file->fd, (*col)->v, (*col)->len );
				FDPRINTF( file->fd, "\"" );
			}
			else if ( oconn->stream == STREAM_COMMA ) {
				file_t *file = (file_t *)oconn->conn;
				FDPRINTF( file->fd, &",\""[ first ] );
				FDNPRINTF( file->fd, (*col)->v, (*col)->len );
			#if 0
				else if ( (*col)->type == T_DATE ) {
					// Try a custom date format?
				}
			#endif
				FDPRINTF( file->fd, "\"" );
			}
			else if ( oconn->stream == STREAM_JSON ) {
				file_t *file = (file_t *)oconn->conn;
				FDPRINTF( file->fd, &",\""[ first ] );
				FDPRINTF( file->fd, (*col)->k );
				FDPRINTF( file->fd, "\": " );
				if ( (*col)->type == T_NULL || ( !(*col)->len && (*col)->type != T_STRING && (*col)->type != T_CHAR ) )
					FDPRINTF( file->fd, "null" );
				else if ( (*col)->type == T_BOOLEAN && (*col)->len && memchr( "Tt1", *(*col)->v, 3 ) )
					FDPRINTF( file->fd, "true" );
				else if ( (*col)->type == T_BOOLEAN && (*col)->len && memchr( "Ff0", *(*col)->v, 3 ) )
					FDPRINTF( file->fd, "false" );
				else if ( (*col)->type == T_INTEGER || (*col)->type == T_DOUBLE ) 
					FDNPRINTF( file->fd, (*col)->v, (*col)->len );
				#if 0
				else if ( (*col)->type == T_BINARY ) /* TODO: Write unicode sequences out */
					FDNPRINTF( file->fd, (*col)->v, (*col)->len );
				#endif
				else {
					FDPRINTF( file->fd, "\"" );
					if ( !memchr( (*col)->v, '"', (*col)->len ) )
						FDNPRINTF( file->fd, (*col)->v, (*col)->len );
					else {
						unsigned char *v = (*col)->v;
						unsigned long len = (*col)->len;	
						for ( ; len; len--, v++ ) {
							// TODO: This could be VERY slow, maybe better to write the blocks at one time?
							( *v == '"' ) ? FDPRINTF( file->fd, "\\\"" ) : FDNPRINTF( file->fd, v, 1 );
						}
					}
					FDPRINTF( file->fd, "\"" );
				}
				( FF.newline ) ? FDPRINTF( file->fd, "\n" ) : 0;
			}
			else if ( oconn->stream == STREAM_CSTRUCT ) {
				file_t *file = (file_t *)oconn->conn;
				FDPRINTF( file->fd, &TAB[9-1] );
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
			}
			else if ( oconn->stream == STREAM_SQL ) {
				file_t *file = (file_t *)oconn->conn;
				( first ) ? 0 : FDPRINTF( file->fd, "," );

				if ( (*col)->type != T_STRING && (*col)->type != T_CHAR )
					FDNPRINTF( file->fd, (*col)->v, (*col)->len );
			#if 0
				else if ( (*col)->type == T_DATE ) {
					// Try a custom date format?
				}
			#endif
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
			else if ( oconn->stream == STREAM_MYSQL ) {
#if 0
FDPRINTF ( 2, (*col)->k ), FDPRINTF ( 2, " = " ), FDNPRINTF( 2, (*col)->v, (*col)->len ), FDPRINTF ( 2, "\n" );
#endif
				// Deref and define
				mysql_t *b = (mysql_t *)oconn->conn;
				MYSQL_BIND *bind = &b->bindargs[ ci ];
				bind->buffer = (*col)->v;
				bind->is_null = NULL;
				bind->length = &(*col)->len;
				//bind->buffer_type = iconn->headers[ ci ]->ptype->ntype; 
				bind->buffer_type = MYSQL_TYPE_STRING;

DPRINTF( "Length of value for %s is: %ld\n", (*col)->k, (*col)->len );
				if ( !(*col)->len && ( (*col)->type == T_INTEGER || (*col)->type == T_DOUBLE || (*col)->type == T_BOOLEAN ) ) {
					//bind->buffer = (char *)null_keyword;
					bind->buffer_type = MYSQL_TYPE_NULL;
					//bind->length = (unsigned long *)&null_keyword_length;
					//bind->buffer_length = 4;
				}

#if 0
				//bind->buffer_length = (*col)->len;
				// Use the approximate type as a base for the format
				if ( (*col)->type == T_NULL )
					bind->buffer_type = MYSQL_TYPE_STRING;
				else if ( (*col)->type == T_BOOLEAN )
					bind->buffer_type = MYSQL_TYPE_TINY;
				else if ( (*col)->type == T_INTEGER )
					bind->buffer_type = MYSQL_TYPE_DECIMAL;
				else if ( (*col)->type == T_DOUBLE )
					bind->buffer_type = MYSQL_TYPE_FLOAT; /* Doubles overflow? */
				else if ( (*col)->type == T_STRING || (*col)->type == T_CHAR )
					bind->buffer_type = MYSQL_TYPE_STRING, bind->buffer_length = (*col)->len;
				else if ( (*col)->type == T_BINARY )
					bind->buffer_type = MYSQL_TYPE_BLOB, bind->buffer_length = (*col)->len;
#endif
#if 0
				if ( (*col)->type == T_STRING || (*col)->type == T_CHAR || (*col)->type == T_BINARY )
					;//bind->buffer_length = (*col)->len;
				else if ( (*col)->type == T_DATE ) {
					MYSQL_TIME *ts = NULL;
					// Allocate a new time structure and write stuff to i
					if ( !( ts = malloc( sizeof( MYSQL_TIME ) ) ) || !memset( ts, 0, sizeof( MYSQL_TIME ) ) ) {
						const char fmt[] = "Allocation error with MYSQL_TIME";
						snprintf( err, errlen, fmt );
						return 0;
					}

					bind->buffer_type = iconn->headers[ ci ]->ptype->ntype;
					bind->buffer = (char *)ts;
					bind->buffer_length = 0;
					bind->length = NULL;
					ts->year = (*col)->date.year;
					ts->month = (*col)->date.month;
					ts->day = (*col)->date.day;
					ts->hour = (*col)->date.hour;
					ts->minute = (*col)->date.minute;
					ts->second = (*col)->date.second;
				}
				// If it's null?  I'm not sure, should be a string anyway
				else {
					// any numeric type will have to be turned into a real value

					// float
					// double
					// any integer type can be cast to a long (hopefully)
				}

				// May have to read and set a double or an integer, etc
				else {
					// Any numeric types?
					//bind->buffer_length = 1;
					bind->length = &iconn->headers[ ci ]->ptype->size;
					bind->buffer = &somenum;
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
		#ifdef BPGSQL_H
			else if ( oconn->stream == STREAM_PGSQL ) {
			#ifdef DEBUG_H
				DPRINTF( "Binding value %p of length %ld at %d\n", (*col)->v, (*col)->len, ci );
			#endif
				pgsql_t *b = (pgsql_t *)oconn->conn;
#if 0
FDPRINTF ( 2, (*col)->k ), FDPRINTF ( 2, " = " ), FDNPRINTF( 2, (*col)->v, (*col)->len ), FDPRINTF ( 2, "\n" );
#endif

DPRINTF( "Length of value for %s (%s) is: %ld\n", (*col)->k, itypes[ (*col)->type ], (*col)->len  );
				// Bind and prepare an insert?
				if ( !(*col)->len && (*col)->type != T_STRING && (*col)->type != T_BINARY && (*col)->type != T_CHAR ) {
					b->bindlens[ ci ] = 0;
					b->bindargs[ ci ] = NULL; //(char *)null_keyword; 
					b->bindfmts[ ci ] = 0;
				}
				else if ( (*col)->type != T_BINARY ) {
					char *v = NULL;
					if ( !( v = malloc( (*col)->len + 1 ) ) || !memset( v, 0, (*col)->len + 1 ) ) {
						const char fmt[] = "Out of memory when binding value at column %s for Postgres";
						snprintf( err, errlen, fmt, iconn->headers[ ci ]->label );
						return 0;
					}
					memcpy( v, (*col)->v, (*col)->len );
					b->bindlens[ ci ] = (*col)->len;
					b->bindargs[ ci ] = v; 
					b->bindfmts[ ci ] = 0;
				}
				else {
					// How do I fetch this differently?
					b->bindargs[ ci ] = (char *)(*col)->v; 
					b->bindlens[ ci ] = (*col)->len;
					b->bindfmts[ ci ] = 1;
				}
#if 0
				else if ( (*col)->type == T_DATE ) {
					//date_t *d = &(*col)->date;	
				} 
#endif
				
			#if 1
				//b->bindoids[ ci ] = 
			#else
				// Use the approximate type as a base for the format
				if ( (*col)->type == T_NULL )
					b->bindoids[ ci ] = PG_TEXTOID; 
				else if ( (*col)->type == T_BOOLEAN )
					b->bindoids[ ci ] = PG_BOOLOID; 
				else if ( (*col)->type == T_STRING || (*col)->type == T_CHAR )
					b->bindoids[ ci ] = PG_TEXTOID; 
				else if ( (*col)->type == T_INTEGER )
					b->bindoids[ ci ] = PG_INT4OID; 
				else if ( (*col)->type == T_DOUBLE )
					b->bindoids[ ci ] = PG_FLOAT4OID; 
				else if ( (*col)->type == T_BINARY )
					b->bindoids[ ci ] = PG_FLOAT4OID;
				else if ( (*col)->type == T_DATE ) {
					b->bindoids[ ci ] = PG_TIMESTAMPOID;
					date_t d = (*col)->date;
					if ( !d.year && !d.month && !d.year ) {
						b->bindoids[ ci ] = PG_TIMEOID;
					}
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
			else if ( oconn->stream == STREAM_SQLITE ) {
			}
		#endif
		} // end for


		/* End the string */
		if ( oconn->type == DB_FILE ) {
			file_t *file = (file_t *)oconn->conn;
			// End the string
			if ( oconn->stream == STREAM_PRINTF )
				;//p_default( 0, ib->k, ib->v );
			else if ( oconn->stream == STREAM_COMMA )
				;//fprintf( oconn->output, " },\n" );
			else if ( oconn->stream == STREAM_CSTRUCT )
				FDPRINTF( file->fd, "}" );
			else if ( oconn->stream == STREAM_CARRAY )
				FDPRINTF( file->fd, " }" );
			else if ( oconn->stream == STREAM_SQL )
				FDPRINTF( file->fd, " );" );
			else if ( oconn->stream == STREAM_JSON ) {
				FDPRINTF( file->fd, "}" ); // count will depend on where in the stream
				( FF.newline ) ? FDPRINTF( file->fd, "\n" ) : 0;
			}
			else if ( oconn->stream == STREAM_XML ) {
				FF.pretty ? FDPRINTF( file->fd, "\t" ) : 0;
				FDPRINTF( file->fd, "</" );
				FDPRINTF( file->fd, oconn->tablename );
				FDPRINTF( file->fd, ">" );
				( FF.newline ) ? FDPRINTF( file->fd, "\n" ) : 0;
			}
			else if ( oconn->stream == STREAM_RAW ) {
				FDPRINTF( file->fd, oconn->rowd );
			}	
		}

	#ifdef BMYSQL_H
		else if ( oconn->stream == STREAM_MYSQL ) {
			mysql_t *b = (mysql_t *)oconn->conn;
		#if 0
			int status = 0;
			status = mysql_bind_param( oconn->conn, iconn->hlen, b->bindargs, NULL );
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
			// TODO: Statements are probably preferred, but its simpler to do it the other way
			// Prepare the statement
			if ( mysql_stmt_prepare( b->stmt, b->query, strlen( b->query ) ) != 0 ) {
				const char fmt[] = "MySQL statement prepare failure: '%s'";
				snprintf( err, errlen, fmt, mysql_stmt_error( b->stmt )  );
				return 0;
			}

			// Use the parameter count to check that everything worked (probably useless)
			if ( mysql_stmt_param_count( b->stmt ) != iconn->hlen ) {
				snprintf( err, errlen, "MySQL column header count mismatch." );
				return 0;
			}

			if ( mysql_stmt_bind_param( b->stmt, b->bindargs ) != 0 ) {
				const char fmt[] = "MySQL bind failure: '%s'";
				snprintf( err, errlen, fmt, mysql_stmt_error( b->stmt )  );
				return 0;
			}

			//printf( "%p ?= %p %d\n", (void *)oconn->typemap, (void *)mysql_map, oconn->typemap == mysql_map );
			if ( mysql_stmt_execute( b->stmt ) != 0 ) {
				const char fmt[] = "MySQL statement exec failure: '%s'";
				snprintf( err, errlen, fmt, mysql_stmt_error( b->stmt ) );
				return 0;
			}

			WPRINTF( "MySQL commit successful.\n" );
		#endif
		}
	#endif

	#ifdef BPGSQL_H
		else if ( oconn->stream == STREAM_PGSQL ) {
			pgsql_t *b = (pgsql_t *)oconn->conn;

			PGresult *r = PQexecParams(
				b->conn,
				b->query,
				iconn->hlen,
				b->bindoids,
				(const char * const *)b->bindargs,
				b->bindlens,
				(int *)b->bindfmts,
				0
			);

			if ( PQresultStatus( r ) != PGRES_COMMAND_OK ) {
				const char fmt[] = "PostgreSQL commit failed: %s";
				snprintf( err, errlen, fmt, PQresultErrorMessage( r ) );
				return 0;
			}

			WPRINTF( "Commit successful, freeing duplicated values\n" );
			PQclear( r );

			// Free all of the allocated values here
			int ii = 0;
			for ( char **bb = b->bindargs; ii < iconn->hlen; bb++, ii++ ) free( *bb );
		}
	#endif

	#if 0
		//Suffix
		if ( oconn->type == DB_FILE ) {
			file_t *file = (file_t *)oconn->conn;
			( FF.suffix ) ? FDPRINTF( file->fd, FF.suffix ) : 0;
			//( FF.newline ) ? FDPRINTF( file->fd, "\n" ) : 0;
		}
	#endif
	}

	return 1;
}



/**
 * close_dsn( dsn_t *conn )
 *
 * Closes a data source: `conn`.
 *
 */
void close_dsn( dsn_t *conn ) {
	WPRINTF( "Attempting to close %s at %p", conn->connstr, (void *)conn->conn );

	if ( conn->conn ) {
		if ( conn->type == DB_FILE ) {
			// Cast
			file_t *file = (file_t *)conn->conn;

			// Free this
			if ( file->start && munmap( (void *)file->start, file->size ) == -1 ) {
				fprintf( stderr, "munmap() %s", strerror( errno ) );
			}

			// Close if the file is not stdout or in
			if ( file->fd > 2 && close( file->fd ) == -1 ) {
				fprintf( stderr, "close() %s", strerror( errno ) );
			}

			free( file );
		}
	#ifdef BSQLITE_H
		else if ( conn->type == DB_SQLITE ) {
			fprintf( stderr, "SQLite3 not done yet.\n" );
		}
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
	}

	if ( conn->connstr ) {
		free( conn->connstr );
	}

	conn->conn = NULL;
}


/**
 * int types_from_dsn( dsn_t * iconn, dsn_t *oconn, config_t *conf, char *err, int errlen ) 
 *
 * Checks that we can support all of the types specified by a query.
 *
 */
int types_from_dsn( dsn_t * iconn, dsn_t *oconn, config_t *conf, char *err, int errlen ) {
	// Define
	char *ename = NULL;
	coerce_t **ctypes = NULL;
	coerce_t **atypes = NULL;

	// Create a map of what a specific type should ALWAYS map to
	if ( conf->wautocast && !( atypes = create_ctypes( conf->wautocast, iconn, oconn, err, errlen ) ) )
		return 0;

	// Create a map of what individual columsn should map to
	if ( conf->wcoerce && !( ctypes = create_ctypes( conf->wcoerce, NULL, oconn, err, errlen ) ) )
		return 0;

	fprintf( stderr, "AUTOCASTED TYPES\n" ), print_ctypes( atypes ), fprintf( stderr, "\n" );
	fprintf( stderr, "COERCED TYPES\n" ), print_ctypes( ctypes ), fprintf( stderr, "\n" );

#if 0
	if ( ov ) {
		int ctypeslen = 0;

		// Multiple coerced types
		if ( memchr( ov, ',', strlen( ov ) ) ) {
			zw_t p = { 0 };
			// TODO: Trim these.  
			for ( coerce_t *t = NULL; strwalk( &p, ov, "," ); t = NULL ) {
				if ( ( t = create_ctype( &ov[ p.pos ], p.size - 1 ) ) ) {
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
			snprintf( err, errlen, "Argument supplied to --coerce is invalid." );
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
#endif

	//
	if ( iconn->type == DB_FILE ) {
		zw_t p;
		int count = 0;
		unsigned int rowlen = 0;
		unsigned int dlen = strlen( delset );
		unsigned char *s = NULL;
		file_t *file = NULL;

		// Catch any silly errors that may have been made on the way back here
		if ( !( file = (file_t *)iconn->conn ) ) {
			const char fmt[] = "mmap() lost reference";
			snprintf( err, errlen, fmt );
			return 0;
		}

		// Find the end of the first row
		for ( s = file->map; ( *s && *s != '\n' && *s != '\r' ); s++ ) {
			rowlen++;
		}

		// Check that col count - 1 == occurrence of delimiters in file
		if ( ( iconn->hlen - 1 ) != ( count = memstrocc( file->map, &delset[2], rowlen ) ) ) {
			const char fmt[] = "Header count and column count differ in file: %s (%d != %d)";
			snprintf( err, errlen, fmt, iconn->connstr, iconn->hlen, count );
			return 0;
		}

		// Check that we have more than 2 lines (so we don't overrun the extract buffer)
		if ( s ) {
			rowlen++, memset( &p, 0, sizeof( zw_t ) );
		}
		else {
			// No terminating \r or \n SHOULDN'T be a problem.  But I'll need to test it...
			// I'll need to also add \0 to the (del)imiter (set)
		}


		// Move through the rest of the entries in the row and approximate types
		for ( int i = 0; memwalk( &p, file->map, (unsigned char *)delset, rowlen, dlen ); i++ ) {
			header_t *st = iconn->headers[ i ];
			char *t = NULL;
			const char *ctype = NULL;
			int len = 0;
			typemap_t *cm = NULL;

			// Check if any of these are looking for a coerced type
			if ( ctypes && ( ctype = find_ctype( ctypes, st->label ) ) ) {
				if ( !( cm = get_typemap_by_nname( oconn->typemap, ctype ) ) ) {
					// TODO: Complete this message
					const char fmt[] = "FILE: Failed to find desired type '%s' at column '%s'";
					snprintf( err, errlen, fmt, ctype, st->label );
					return 0;
				}
			}
			else if ( atypes ) {

			}

			// TODO: Assuming a blank value is a string might not always be the best move
			if ( !p.size || !( p.size - 1 ) ) {
				st->type = T_STRING;
				st->ptype = get_typemap_by_btype( oconn->typemap, T_STRING );
				continue;
			}

			// Try to copy again
			if ( !( t = copy( &((char *)file->map)[ p.pos ], p.size - 1, &len, CASE_NONE ) ) ) {
				// TODO: This isn't very clear, but it is unlikely we'll ever run into it...
				const char fmt[] = "FILE: Encountered memory bound at line 2, column %d";
				snprintf( err, errlen, fmt, i );
				return 0;
			}

			// Handle NULL or empty strings
			if ( !len ) {
				st->type = T_STRING;
				st->ptype = get_typemap_by_btype( oconn->typemap, T_STRING );
				continue;
			}

			// This will be the default type
			st->type = get_type( (unsigned char *)t, T_STRING, len );
			st->ptype = ( cm ) ? cm : get_typemap_by_btype( oconn->typemap, st->type );
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
		const char eng[] = "MySQL";

		for ( int i = 0, fcount = mysql_field_count( b->conn ); i < fcount; i++ ) {
			MYSQL_FIELD *f = mysql_fetch_field_direct( b->res, i );
			header_t *st = iconn->headers[ i ];
			const char *name = f->name;
			const char *ctypename = NULL;
			typemap_t *nm = NULL, *cm = NULL, *am = NULL;

			// Find the native typemap
			// TODO: If unsupported, it probably should be null and I either 
			// coerce or guess instead of just dying...
			if ( !( nm = get_typemap_by_ntype( iconn->typemap, f->type ) ) ) {
				snprintf( err, errlen, "Unsupported %s type received: %d", eng, f->type );
				return 0;
			}

			// Check if there are any matching coerced types for this column
			if ( ctypes && ( ctypename = find_ctype( ctypes, name ) ) ) {
				cm = get_typemap_by_nname( oconn->typemap, ctypename );
				if ( !cm ) {
					const char fmt[] = "%s: Failed to find desired type '%s' for column '%s' for supported %s types ";
					snprintf( err, errlen, fmt, eng, ctypename, name, ename );
					return 0;
				}
			}
			// Check if any --auto rules have been declared 
			else if ( atypes && ( ctypename = find_atype( atypes, nm->name ) ) ) {
				cm = get_typemap_by_nname( oconn->typemap, ctypename );
				if ( !cm ) {
					const char fmt[] = "%s: Auto conversion to type '%s' failed for column '%s'";
					snprintf( err, errlen, fmt, eng, ctypename, name );
					return 0;
				}
			}

			// This can still be useful...
			FPRINTF( "Got coerced type %s for column '%s'\n", ctypename, name );

			// Set the native type always
			st->ntype = nm;

			// Now, we set the preferred type
			if ( cm )
				st->ptype = cm, st->type = cm->basetype;
			else if ( iconn->type == oconn->type )
				st->ptype = nm, st->type = nm->basetype;
			else {
				// Different output type, and no coercion, so find the closest match
				DPRINTF( "BASE TYPE IS %d %s\n", nm->basetype, itypes[ nm->basetype ] );
				if ( !( am = get_typemap_by_btype( oconn->typemap, nm->basetype ) ) ) {
					const char fmt[] = "%s: Failed to find matching type "
						"for column '%s', type '%s' for supported %s types ";
					snprintf( err, errlen, fmt, eng, name, "x", "x" );
					return 0;
				}
				st->ptype = am;
				st->type = nm->basetype; //am->basetype would work too
			}
		
		#if 0
			st->maxlen = 0;
			st->precision = 0;
			st->filter = 0;
			st->date = 0;
		#endif
		}
	}
#endif

#ifdef BPGSQL_H
	else if ( iconn->type == DB_POSTGRESQL ) {
		pgsql_t *b = (pgsql_t *)iconn->conn;
		const char eng[] = "PostgreSQL";
		for ( int i = 0, fcount = PQnfields( b->res ); i < fcount; i++ ) {
			header_t *st = iconn->headers[ i ];
			const char *name = PQfname( b->res, i );
			const char *ctypename = NULL;
			Oid pgtype = PQftype( b->res, i );
			typemap_t *nm = NULL, *cm = NULL, *am = NULL;

			// Find the native typemap
			// TODO: If unsupported, it probably should be null and I either 
			// coerce or guess instead of just dying...
			if ( !( nm = get_typemap_by_ntype( iconn->typemap, pgtype ) ) ) {
				snprintf( err, errlen, "Unsupported %s type received: %d", eng, pgtype );
				return 0;
			}

			// Check if there are any matching coerced types for this column
			if ( ctypes && ( ctypename = find_ctype( ctypes, name ) ) ) {
				cm = get_typemap_by_nname( oconn->typemap, ctypename );
				if ( !cm ) {
					const char fmt[] = "%s: Failed to find desired type '%s' for column '%s' for supported %s types ";
					snprintf( err, errlen, fmt, eng, ctypename, name, ename );
					return 0;
				}
			}
			// Check if any --auto rules have been declared 
			else if ( atypes && ( ctypename = find_atype( atypes, nm->name ) ) ) {
				cm = get_typemap_by_nname( oconn->typemap, ctypename );
				if ( !cm ) {
					const char fmt[] = "%s: Auto conversion to type '%s' failed for column '%s'";
					snprintf( err, errlen, fmt, eng, ctypename, name );
					return 0;
				}
			}

			// This can still be useful...
			FPRINTF( "Got coerced type %s for column '%s'\n", ctypename, name );


			// Set the native type always
			st->ntype = nm;

			// Now, we set the preferred type
			if ( cm )
				st->ptype = cm, st->type = cm->basetype; 		
			else if ( iconn->type == oconn->type )
				st->ptype = nm, st->type = nm->basetype; 		
			else {
				// Different output type, and no coercion, so find the closest match
				if ( !( am = get_typemap_by_btype( oconn->typemap, nm->basetype ) ) ) {
					const char fmt[] = "%s: Failed to find matching type "
						"for column '%s', type '%s' in supported %s types ";
					snprintf( err, errlen, fmt, eng, name, nm->name, get_conn_type( oconn->type ) );
					return 0;
				}
				st->ptype = am;
				st->type = nm->basetype;					
			}
		
		#if 0
			st->maxlen = 0;
			st->precision = 0;
			st->filter = 0;
			st->date = 0; // The timezone data?
		#endif
		}

	}
#endif

	free_ctypes( atypes ), free_ctypes( ctypes );
	return 1;
}


/**
 * int scaffold_dsn ( config_t *conf, dsn_t *ic, dsn_t *oc, char *err, int errlen  ) 
 *
 * This is intended to fill out a dsn_t manually when we already have an input 
 *
 */
int scaffold_dsn ( config_t *conf, dsn_t *ic, dsn_t *oc, char *err, int errlen  ) {

	// No connection string was specified, so we can make a few assumptions.
	if ( !oc->connstr ) {
		// Output source is going to be stdout
		oc->type = DB_FILE;
		oc->connstr = strdup( "/dev/stdout" );
		//oc->stream = STREAM_PRINTF;
		oc->typemap = default_map;

		if ( conf->wtable ) {
			if ( strlen( conf->wtable ) >= sizeof( oc->tablename ) ) {
				snprintf( err, errlen, "Table name '%s' is too long for buffer.", conf->wtable );
				return 0;
			}
			snprintf( oc->tablename, sizeof( oc->tablename ), "%s", conf->wtable );
		}
		else if ( strlen( ic->tablename ) > 0 ) {
			snprintf( oc->tablename, sizeof( oc->tablename ), "%s", ic->tablename );
		}

		// I'll need a table name depending on other options
	#if 0
		if ( config->wschema && !config->conf->wtable && !strlen( ic->tablename ) ) {
			snprintf( err, errlen, "Wanted --schema, but table name is invalid or unspecified." );
			return 0;
		}
	#endif

		// If we're reading from a database input source make some assumptions
		if ( ic->type == DB_MYSQL ) {
			DPRINTF( "Input is MySQL, but no output source specified.  Using MySQL as default map.\n" );
			oc->typemap = mysql_map;
		}
		else if ( ic->type == DB_POSTGRESQL ) {
			DPRINTF( "Input is Postgres, but no output source specified.  Using Postgres as default map.\n" );
			oc->typemap = pgsql_map;
		}
		else {
			DPRINTF( "No output source specified, using default map.\n" );
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
	if ( conf->wtable /* config->conf->wtable */ ) {
		if ( strlen( conf->wtable ) >= sizeof( oc->tablename ) ) {
			snprintf( err, errlen, "Table name '%s' is too long for buffer.", conf->wtable );
			return 0;
		}
		snprintf( oc->tablename, sizeof( oc->tablename ), "%s", conf->wtable );
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
	for ( header_t **s = conn->headers; s && *s; s++ ) {
		printf( "%s\n", (*s)->label );
	}
	destroy_dsn_headers( conn );
	return 1;
}



/**
 * void free_ctypes( coerce_t **ctypes ) 
 *
 * Free allocate coerce_t arrays.
 *
 */
void free_ctypes( coerce_t **ctypes ) {
	if ( ctypes ) {
		for ( coerce_t **x = ctypes; x && *x; x++ ) free( *x ); 
		free( ctypes );
	}
}




/**
 * int help() 
 *
 * Help message.  Can turn this into an actual function
 * and leave the `struct help` at the top of this file.
 *
 */
int help () {
	const char *fmt = "%-2s%s --%-24s%-30s\n";
	struct help { const char *sarg, *larg, *desc; } msgs[] = {
		{ "-i", "input <arg>", "Specify an input datasource (required)"  },
		{ ""  , "create",      "If an output source does not exist, create it"  },
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
		{ "-R", "row-delimiter <arg>", "Specify a custom row delimiter when using --raw option" },
		{ "-N", "column-delimiter <arg>", "Specify a custom column delimiter when using --raw option" },
		{ "-u", "output-delimiter <arg>", "Specify an output delimiter when generating serialized output" },
	#if 1
		{ "-t", "typesafe",    "Enforce and/or enable typesafety" },
		{ "-C", "coerce",      "Specify a type for a column" },
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
		{ "-n", "newline",     "Generate a newline after each row." },
		{ "",   "id <arg>",    "Add and specify a unique ID column named <arg>."  },
		{ "",   "add-datestamps","Add columns for date created and date updated"  },
		{ "-a", "ascii",       "Remove any characters that aren't ASCII and reproduce"  },
		{ "",   "no-unsigned", "Remove any unsigned character sequences."  },
		{ "-T", "table <arg>", "Use the specified table when connecting to a database for source data"  },
		{ "-Q", "query <arg>", "Use this query to filter data from datasource"  },
		{ "-y", "stats",       "Dump stats at the end of an operation"  },
#if 0
		{ "-D", "datasource <arg>",   "Use the specified connection string to connect to a database for source data"  },
		{ "-L", "limit <arg>",   "Limit the result count to <arg>"  },
		{ "",   "buffer-size <arg>",  "Process only this many rows at a time when reading source data"  },
#endif
		{ "-A", "auto",        "Set a default coercion for a specific type (e.g. blob=text)" },
		{ "-X", "dumpdsn",     "Dump the DSN only. (DEBUG)" },
		{ "-h", "help",        "Show help." },
	};

	for ( int i = 0; i < sizeof(msgs) / sizeof(struct help); i++ ) {
		struct help h = msgs[i];
		fprintf( stderr, fmt, h.sarg, strlen( h.sarg ) ? "," : " ", h.larg, h.desc );
	}
	return 0;
}




/**
 * config_t config
 *
 * The options in the config.
 *
 */
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
	.wtable = NULL,
	.wrowd = "\n",
	.wcold = ",",
};



#ifdef DEBUG_H
/**
 * void print_date( date_t *date ) 
 *
 * Print a somewhat formatted date.
 *
 */
void print_date( date_t *date ) {
	fprintf( stderr, "Y: %d, M: %d, D: %d, H: %d, m: %d, s:%d\n", 
		date->year, date->month, date->day, date->hour, date->minute, date->second 
	);
}

/**
 * void print_dsn ( dsn_t *conninfo ) 
 *
 * Dump the datasource info.
 *
 */
void print_dsn ( dsn_t *conninfo ) {
	fprintf( stderr, "%-20s= %s\n", "connstr", conninfo->connstr );
	fprintf( stderr, "%-20s= %s\n", "dbtype",   dbtypes[ conninfo->type ] );
	fprintf( stderr, "%-20s= %s\n", "username", conninfo->username );
	fprintf( stderr, "%-20s= %s\n", "password", conninfo->password );
	fprintf( stderr, "%-20s= %s\n", "hostname", conninfo->hostname );
	fprintf( stderr, "%-20s= %s\n", "dbname", conninfo->dbname );
	fprintf( stderr, "%-20s= %s\n", "tablename", conninfo->tablename );
	fprintf( stderr, "%-20s= %d\n", "port", conninfo->port );
	fprintf( stderr, "%-20s= %p\n", "conn", conninfo->conn );
	fprintf( stderr, "%-20s= %s\n", "connoptions", conninfo->connoptions );
	fprintf( stderr, "%-20s= %p\n", "typemap", (void *)conninfo->typemap );

	if ( conninfo->typemap == default_map )
		fprintf( stderr, "(default)\n" );
	else if ( conninfo->typemap == sqlite3_map )
		fprintf( stderr, "(SQLite)\n" );
	else if ( conninfo->typemap == mysql_map )
		fprintf( stderr, "(MySQL)\n" );
	else if ( conninfo->typemap == pgsql_map ) {
		fprintf( stderr, "(PostgreSQL)\n" );
	}

	//if headers have been initialized show me that
	fprintf( stderr, "%-20s= %p\n", "headers", (void *)conninfo->headers );
	if ( conninfo->headers ) {
		fprintf( stderr, "  %-20s [%s, %s, %s, %s]\n", "label", "C", "N", "B", "P" );
		for ( header_t **h = conninfo->headers; h && *h; h++ ) {
			fprintf( stderr, "  %-20s [%s, %s, %s, %s]\n",
				(*h)->label,
				(*h)->ctype ? (*h)->ctype->name : "(nil)",
				(*h)->ntype ? (*h)->ntype->name : "(nil)",
				(*h)->ptype ? itypes[ (*h)->ptype->basetype ] : "(nil)",
				(*h)->ptype ? (*h)->ptype->name : "(nil)"
			);
		}
	}
}


/**
 * void print_headers( dsn_t *t ) 
 *
 * Dump all of the headers in a datasource.
 *
 */
void print_headers( dsn_t *t ) {
	for ( header_t **s = t->headers; s && *s; s++ ) {
		printf( "%s %d\n", (*s)->label, (*s)->type );
	}
}


/**
 * void print_stream_type( stream_t t ) 
 *
 * Dump all of the different stream types.
 *
 */
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


/**
 * void print_config( config_t *config ) 
 *
 * Dump the given configuration.
 *
 */
void print_config( config_t *config ) {
	fprintf( stderr, "%-20s= %d\n", "wstream", config->wstream );
	fprintf( stderr, "%-20s= %d\n", "wdeclaration", config->wdeclaration );
	fprintf( stderr, "%-20s= %d\n", "wschema", config->wschema );
	fprintf( stderr, "%-20s= %d\n", "wheaders", config->wheaders );
	fprintf( stderr, "%-20s= %d\n", "wconvert", config->wconvert );
	fprintf( stderr, "%-20s= %d\n", "wclass", config->wclass );
	fprintf( stderr, "%-20s= %d\n", "wstruct", config->wstruct );
	fprintf( stderr, "%-20s= %d\n", "wtypesafe", config->wtypesafe );
	fprintf( stderr, "%-20s= %d\n", "wnewline", config->wnewline );
	fprintf( stderr, "%-20s= %d\n", "wdatestamps", config->wdatestamps );
	fprintf( stderr, "%-20s= %d\n", "wnounsigned", config->wnounsigned );
	fprintf( stderr, "%-20s= %d\n", "wspcase", config->wspcase );
	fprintf( stderr, "%-20s= %s\n", "wid", config->wid );
	fprintf( stderr, "%-20s= %d\n", "wstats", config->wstats );
	fprintf( stderr, "%-20s= %s\n", "wcutcols", config->wcutcols );
	fprintf( stderr, "%-20s= %s\n", "widname", config->widname );
	fprintf( stderr, "%-20s= %s\n", "wcoerce", config->wcoerce );
	fprintf( stderr, "%-20s= %s\n", "wquery", config->wquery );
	fprintf( stderr, "%-20s= %s\n", "wprefix", config->wprefix );
	fprintf( stderr, "%-20s= %s\n", "wsuffix", config->wsuffix ); 
	fprintf( stderr, "%-20s= %s\n", "wtable", config->wtable ); 
}


/**
 * void print_ctypes( coerce_t **ctypes ) 
 *
 * Dump the coerced types.
 *
 */
void print_ctypes( coerce_t **ctypes ) {
	for ( coerce_t **x = ctypes; x && *x; x++ ) {
		fprintf( stderr, "%s => %s\n", (*x)->misc, (*x)->typename );
	}
}


#endif


int main ( int argc, char *argv[] ) {
	SHOW_COMPILE_DATE();

	// Define an input source, and an output source
	char err[ ERRLEN ];
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
	output.stream = STREAM_PRINTF;

	// Evaluate help or missing options
	if ( argc < 2 || !strcmp( *argv, "-h" ) || !strcmp( *argv, "--help" ) ) {
		return help();
	}

	for ( ++argv; *argv; ) {

		// TODO: If possible, macro the strcmps & combine the flag condition.  Extra info can be passed into whatever function this is.
		if ( !strcmp( *argv, "--no-unsigned" ) )
			config.wnounsigned = 1;
		else if ( !strcmp( *argv, "--add-datestamps" ) )
			config.wdatestamps = 1;
		else if ( EVALARG( *argv, "-n", "--newline" ) )
			config.wnewline = 1;
		else if ( EVALARG( *argv, "-r", "--raw" ) )
			config.wconvert = 1, output.stream = STREAM_RAW;
		else if ( EVALARG( *argv, "-j", "--json" ) )
			config.wconvert = 1, output.stream = STREAM_JSON;
		else if ( EVALARG( *argv, "-x", "--xml" ) )
			config.wconvert = 1, output.stream = STREAM_XML; 
		else if ( EVALARG( *argv, "-q", "--sql" ) )
			config.wconvert = 1, output.stream = STREAM_SQL;
		else if ( !strcmp( *argv, "--camel-case" ) )
			config.wspcase = CASE_CAMEL;
		else if ( EVALARG( *argv, "-S", "--schema" ) )
			config.wschema = 1;
		else if ( EVALARG( *argv, "-H", "--headers" ) )
			config.wheaders = 1;
		else if ( EVALARG( *argv, "-c", "--convert" ) )
			config.wconvert = 1;
		else if ( !strcmp( *argv, "--class" ) )
			config.wclass = 1, config.wspcase = CASE_CAMEL;
		else if ( EVALARG( *argv, "-y", "--stats" ) )
			config.wstats = 1;
	#ifdef DEBUG_H
		else if ( DEVAL( EVALARG( *argv, "-X", "--dumpdsn" ) ) ) 
			config.wdumpdsn = 1;
	#endif
		else if ( EVALARG( *argv, "-R", "--row-delimiter" ) && !SAVEARG( argv, config.wrowd ) )
			return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --coerce." );
		else if ( EVALARG( *argv, "-N", "--column-delimiter" ) && !SAVEARG( argv, config.wcold ) )
			return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --coerce." );
		else if ( EVALARG( *argv, "-C", "--coerce" ) && !SAVEARG( argv, config.wcoerce ) )
			return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --coerce." );
		else if ( EVALARG( *argv, "-A", "--auto" ) && !SAVEARG( argv, config.wautocast ) )
			return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --auto." );
		else if ( !strcmp( *argv, "--id" ) && !SAVEARG( argv, config.wid ) )
			return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --id." );
		else if ( EVALARG( *argv, "-f", "--format" ) && !SAVEARG( argv, config.wstype ) )
			return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --format." );
		else if ( EVALARG( *argv, "-p", "--prefix" ) && !SAVEARG( argv, config.wprefix ) )
			return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --prefix." );
		else if ( EVALARG( *argv, "-x", "--suffix" ) && !SAVEARG( argv, config.wsuffix ) )
			return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --suffix." );
		else if ( EVALARG( *argv, "-T", "--table" ) && !SAVEARG( argv, config.wtable ) )
			return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --table." );
		else if ( EVALARG( *argv, "-Q", "--query" ) && !SAVEARG( argv, config.wquery ) )
			return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --query." );
		else if ( EVALARG( *argv, "-i", "--input" ) && !SAVEARG( argv, input.connstr ) )
			return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --input." );
		else if ( EVALARG( *argv, "-o", "--output" ) && !SAVEARG( argv, output.connstr ) )
			return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --output." );
		else if ( EVALARG( *argv, "-d", "--delimiter" ) ) {
			char *del = *(++argv);
			if ( !del )
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --delimiter." );
			else if ( *del == 9 || ( strlen( del ) == 2 && !strcmp( del, "\\t" ) ) )
				DELIM = "\t";
			else if ( !dupval( del, &DELIM ) ) {
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --delimiter." );
			}
		}
		else if ( EVALARG( *argv, "-u", "--output-delimiter" ) ) {
			//Output delimters can be just about anything,
			//so we don't worry about '-' or '--' in the statement
			if ( ! *( ++argv ) )
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --output-delimiter." );
			else {
				// TODO: Highly consider moving this
				// od = strdup( *argv );
				char *a = memchr( *argv, ',', strlen( *argv ) );
				if ( !a )
					FF.leftdelim = *argv, FF.rightdelim = *argv;
				else {
					*a = '\0';
					FF.leftdelim = *argv;
					FF.rightdelim = ++a;
				}
			}
		}
		else if ( !strcmp( *argv, "--for" ) ) {
			if ( output.connstr )
				return ERRPRINTF( ERRCODE, "%s\n", "Can't specify both --for & --output" );

			if ( *( ++argv ) == NULL )
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for flag --for." );

			if ( !strcasecmp( *argv, "sqlite3" ) )
				config.wdbengine = SQL_SQLITE3, output.connstr = "sqlite3://";
		#ifdef BPGSQL_H
			else if ( !strcasecmp( *argv, "postgres" ) )
				config.wdbengine = SQL_POSTGRES, output.connstr = "postgres://";
		#endif
		#ifdef BMYSQL_H
			else if ( !strcasecmp( *argv, "mysql" ) )
				config.wdbengine = SQL_MYSQL, output.connstr = "mysql://";
		#endif
		#if 0
			else if ( !strcasecmp( *argv, "mssql" ) )
				config.wdbengine = SQL_MSSQLSRV;
			else if ( !strcasecmp( *argv, "oracle" ) ) {
				config.wdbengine = SQL_ORACLE;
				return ERRPRINTF( ERRCODE, "Oracle SQL dialect is currently unsupported, sorry...\n" );
			}
		#endif
			else {
				return ERRPRINTF( ERRCODE, "Argument specified '%s' for flag --for is an invalid option.", *argv );
			}

		}
#if 0
		else if ( !strcmp( *argv, "--struct" ) ) {
			structdata = 1;
			serialize = 1;
			if ( !dupval( *( ++argv ), &input.connstr ) ) {
				return ERRPRINTF( ERRCODE, "%s\n", "No argument specified for --struct." );
			}
		}
#endif
		else if ( *(*argv) == '-' ) {
			fprintf( stderr, "Got unknown argument: %s\n", *argv );
			return help();
		}

		argv++;
		// Could evaluate again here...
	}

	// Do some basic checks on given options
	if ( config.wstype && check_for_valid_stream( config.wstype ) == -1 ) {
		return ERRPRINTF( ERRCODE, "Selected stream '%s' is invalid.", config.wstype );
	}

	// The datasource names are dynamically allocated right now.
	// Stick to that until we can get back to it...
	// TODO: Connection strings should be statically allocated
	if ( input.connstr )
		input.connstr = strdup( input.connstr );

	if ( output.connstr )
		output.connstr = strdup( output.connstr );

#ifdef DEBUG_H
	// We keep this for testing
	if ( config.wdumpdsn ) {
		if ( !parse_dsn_info( &input, err, sizeof( err ) ) ) {
			close_dsn( &input );
			return ERRPRINTF( ERRCODE, "Input source is invalid: %s\n", err );
		}

		if ( !parse_dsn_info( &output, err, sizeof( err ) ) ) {
			close_dsn( &output );
			return ERRPRINTF( ERRCODE, "Output source is invalid: %s\n", err );
		}

		close_dsn( &input ), close_dsn( &output );
	}
#endif

	// Start the timer here
	if ( config.wstats )
		clock_gettime( CLOCK_REALTIME, &stimer );

	// If the user didn't actually specify an action, we should close and tell them that
	// what they did is useless.
	if ( !config.wconvert && !config.wschema && !config.wheaders )
		return ERRPRINTF( ERRCODE, "No action specified, exiting.\n" );

	// The dsn is always going to be the input
	if ( !parse_dsn_info( &input, err, sizeof( err  ) ) ) {
		close_dsn( &input );
		return ERRPRINTF( ERRCODE, "Input source is invalid: %s\n", err );
	}

	// If the user specified a table, fill out 
	if ( config.wtable && strlen( config.wtable ) < sizeof( input.tablename ) )
		snprintf( input.tablename, sizeof( input.tablename ), "%s", config.wtable );

	// Open the DSN first (regardless of type)
	if ( !open_dsn( &input, &config, err, sizeof( err ) ) ) {
		close_dsn( &input );
		return ERRPRINTF( ERRCODE, "DSN open failed: %s\n", err );
	}

	// Create the header_t here
	if ( !headers_from_dsn( &input, &config, err, sizeof( err ) ) ) {
		close_dsn( &input );
		destroy_dsn_headers( &input );
		return ERRPRINTF( ERRCODE, "Header creation failed: %s.\n", err );
	}

	// Create the headers only
	if ( config.wheaders ) {
		cmd_headers( &input );
		close_dsn( &input );
		return 0;
	}

	// Scaffolding can fail
	if ( !scaffold_dsn( &config, &input, &output, err, sizeof( err ) ) ) {
		close_dsn( &input );
		close_dsn( &output );
		return ERRPRINTF( ERRCODE, "%s", err );
	}

	// The input datasource has to be prepared too
	if ( !prepare_dsn_for_read( &input, err, sizeof( err ) ) ) {
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
	if ( !types_from_dsn( &input, &output, &config, err, sizeof( err ) ) ) {
		destroy_dsn_headers( &input );
		close_dsn( &input );
		close_dsn( &output );
		return ERRPRINTF( ERRCODE, "typeget failed: %s\n", err );
	}

	// Create a schema
	if ( config.wschema ) {
		if ( !schema_from_dsn( &input, &output, schema_fmt, MAX_STMT_SIZE, err, sizeof( err ) ) ) {
			destroy_dsn_headers( &input );
			close_dsn( &output );
			close_dsn( &input );
			return ERRPRINTF( ERRCODE, "Schema build failed: %s\n", err );
		}

		// Dump said schema
		fprintf( stdout, "%s", schema_fmt );

		// Close the output source here
		close_dsn( &output );
	}

	else if ( config.wconvert ) {
		// Check the stream
		if ( !output.stream ) {
			if ( output.type == DB_MYSQL )
				output.stream = STREAM_MYSQL;
			else if ( output.type == DB_POSTGRESQL ) {
				output.stream = STREAM_PGSQL;
			}
		}

#if 1
	#if 1
//print_dsn( &output ); getchar();
		// Test and try to create if it does not exist
		if ( !test_dsn( &output, err, sizeof( err ) ) && !create_dsn( &input, &output, err, sizeof( err ) ) ) {
			//close_dsn( &output );
			destroy_dsn_headers( &input );
			close_dsn( &input );
			return ERRPRINTF( ERRCODE, "Output DSN '%s' is inaccessible or could not be created: %s\n", output.connstr, err );
		}
	#else
		// Create the output DSN if it does not exist
		if ( !create_dsn( &output, &input, err, sizeof( err ) ) ) {
			fprintf( stderr, "Creating output DSN failed: %s\n", err );
			return 1;
		}
	#endif
#endif

		// Open the output dsn
		if ( !open_dsn( &output, &config, err, sizeof( err ) ) ) {
			destroy_dsn_headers( &input );
			close_dsn( &input );
			return ERRPRINTF( ERRCODE, "DSN open failed: %s\n", err );
		}

#if 0
	fprintf( stderr, "[ INPUT ]\n" ), print_dsn( &input );	
	fprintf( stderr, "[ OUTPUT ]\n" ), print_dsn( &output );	
	fprintf( stderr, "[ CONFIG ]\n" ), print_config( &config );	
#endif

		// Try to prepare
		if ( !prepare_dsn_for_write( &output, &input, err, sizeof( err ) ) ) {
			destroy_dsn_headers( &input );
			close_dsn( &input );
			return ERRPRINTF( ERRCODE, "Prepare output DSN failed: %s\n", err );
		}


		// Set these automatically... 
		if ( output.stream == STREAM_RAW ) {
			output.cold = config.wcold;	
			output.rowd = config.wrowd;	
		}

#if 0
fprintf( stderr, "IN:\n" ),
print_dsn( &input ), 
fprintf( stderr, "\nOUT:\n" ),
print_dsn( &output );
#endif

		// Need to write a "pre" node in some cases

		// Control the streaming / buffering from here
		for ( int i = 0; i < 1; i++ ) {

			// Stream to records
			if ( !records_from_dsn( &input, 1000, 0, err, sizeof( err ) ) ) {
				destroy_dsn_headers( &input );
				close_dsn( &input );
				return ERRPRINTF( ERRCODE, "Failed to generate records from data source: %s.\n", err );
			}

			#if 0
			for ( row_t **row = input.rows; row && *row; row++ ) {
				fprintf( stderr, "**row - Why is it not? %p\n", (void *)*row ), getchar();
			}
			#endif

			// Do the transport (buffering can be added pretty soon)
			if ( !transform_from_dsn( &input, &output, 0, 0, err, sizeof(err) ) ) {
				fprintf( stderr, "Failed to transform records from data source: %s\n", err );
				destroy_dsn_headers( &input );
				close_dsn( &input );
				return 1;
			}
			// Destroy the rows
			destroy_dsn_rows( &input );
		} // end for

		// once we get to the end, "dismount" whatever preparations we made
		unprepare_dsn( &output );

		// close our open data source
		close_dsn( &output );
	}

	// Show the stats if we did that
	if ( config.wstats ) {
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
