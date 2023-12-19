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

#include "vendor/zwalker.h"
#include "vendor/util.h"

/* Optionally include support for MySQL */
#define BMYSQL_H
#ifdef BMYSQL_H
 #include <mysql.h>
#endif

/* Optionally include support for Postgres */
#define BPGSQL_H
#ifdef BPGSQL_H
 #include <libpq-fe.h>
#endif

#define nerr( ... ) \
	(fprintf( stderr, "briggs: " ) ? 1 : 1 ) && (fprintf( stderr, __VA_ARGS__ ) ? 0 : 0 )

#define SHOW_COMPILE_DATE() \
	fprintf( stderr, "briggs v" VERSION " compiled: " __DATE__ ", " __TIME__ "\n" )

#define CHECK_MYSQL_TYPE( type, type_array ) \
	memchr( (unsigned char *)type_array, type, sizeof( type_array ) )

#ifdef WIN32
 #define NEWLINE "\r\n"
#else
 #define NEWLINE "\n"
#endif

#ifdef DEBUG_H
 #define DPRINTF( ... ) fprintf( stderr, __VA_ARGS__ )
#else
 #define DPRINTF( ... ) 0
#endif

#ifdef BMYSQL_H
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
	MYSQL_TYPE_FLOAT,  
	MYSQL_TYPE_DOUBLE,
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
#ifdef BPGSQL_H
, STREAM_PGSQL
#endif
#ifdef BMYSQL_H
, STREAM_MYSQL
#endif
} stream_t;


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
	/* T_BINARY // If integrating this, consider using a converter */
} type_t;


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
	SQL_POSTGRES,
	SQL_ORACLE,
	SQL_MSSQLSRV,
	SQL_MYSQL
} sqltype_t;



/**
 * dbtype_t 
 *
 * Choose a database type 
 *
 */
typedef enum {
	DB_NONE,
	DB_MYSQL,
	DB_POSTGRESQL,
	DB_SQLITE
} dbtype_t;



/**
 * schema_t 
 *
 * General format for headers or column names coming from a datasource
 *
 */
typedef struct schema_t {
	char label[ 128 ];
	type_t type; 
	int primary;
	int options;
} schema_t;



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
	int len;
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
	int type;
	int port;
	int options;	
	//char socketpath[ 2048 ];
	char *connstr;
	char *connoptions;
	//FILE *file;
	void *conn;
	void *res;
	schema_t **headers;
	row_t **rows;
	int hlen;
	int rlen;
	int clen;
	char **defs;	

	FILE *output;
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



// Forward declarations for different output formats
void p_default( int, column_t * );
void p_xml( int, column_t * );
void p_comma( int, column_t * );
void p_cstruct( int, column_t * );
void p_carray( int, column_t * );
void p_sql( int, column_t * );
void p_json( int, column_t * );
void p_pgsql( int, column_t * );
void p_mysql( int, column_t * );




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
int limit = 0;
int headers_only = 0;
int convert = 0;
int ascii = 0;
int newline = 1;
int typesafe = 0;
int schema = 0;
int hlen = 0;
int no_unsigned = 0;
int adv = 0;
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
	char *a = malloc( size + 1 );
	int nsize = 0;
	memset( a, 0, size + 1 );
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



#ifdef BPGSQL_H
// Write a Postgres row
void p_pgsql ( int i, column_t *r ) {
return;
}
#endif



#ifdef BMYSQL_H
// Write a MySQL row
void p_mysql ( int i, column_t *r ) {
return;
}
#endif



// Extract the headers from a CSV
char ** generate_headers( char *buf, char *del ) {
	char **headers = NULL;	
	zWalker p = { 0 };

	for ( int hlen = 0; strwalk( &p, buf, del ); ) {
		char *t = ( !p.size ) ? no_header : copy( &buf[ p.pos ], p.size - 1, case_type );
		add_item( &headers, t, char *, &hlen );
		if ( p.chr == '\n' || p.chr == '\r' ) break;
	}

	return headers;
}



// Extract the headers from a CSV
schema_t ** generate_headers_from_text( char *buf, char *del ) {
	schema_t **hxlist = NULL;	
	zWalker p = { 0 };

	for ( int hlen = 0; strwalk( &p, buf, del ); ) {
		char *t = ( !p.size ) ? no_header : copy( &buf[ p.pos ], p.size - 1, case_type );
		schema_t *hx = malloc( sizeof( schema_t ) );
		memset( hx, 0, sizeof( schema_t ) );
		snprintf( hx->label, sizeof( hx->label ), "%s", t );
		add_item( &hxlist, hx, schema_t *, &hlen );
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


#if 0
static unsigned char * getvalue ( unsigned char *src, int size ) {
	// Define stuff
	unsigned char *dest = NULL;
	unsigned char *pp = src;
	unsigned int k = 0;

	// Allocate
	if ( !( dest = malloc( size ) ) || !memset( dest, 0, size ) ) {
		return NULL;
	}

	// Copy off a raw count of whatever is in the column to destination
	if ( !reps && !no_unsigned ) {
		memcpy( dest, pp, size - 1 );
		return dest;
	}

	// Copy only PRINTABLE ascii characters
	if ( no_unsigned ) {
		for ( int i = 0; i < size; i++ ) {
			if ( pp[ i ] < 32 || pp[ i ] > 126 )
				;// v->v[ i ] = ' ';
			else {
				dest[ k ] = pp[ i ];
				k++;
			}
		}
	}

	if ( reps ) {
		if ( !k )
			k = size;
		else {
			pp = dest; //v->v;
		}

		for ( unsigned int i = 0, l = 0; i < k; i++ ) {
			int j = 0;
			struct rep **r = reps;
			while ( (*r)->o ) {
				if ( pp[ i ] == (*r)->o ) {
					j = 1;		
					if ( (*r)->r != 0 ) {
						dest[ l ] = (*r)->r;
						l++;
					}
					break;
				}
				r++;
			}
			if ( !j ) {
				dest[ l ] = pp[ i ];
				l++;
			}
		}
	}

	return dest;
}
#endif


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



// Destroy inner set of records
void free_inner_records( record_t **v ) {
	record_t **iv = v;	
	while ( *iv ) {
		free( (*iv)->v );
		free( *iv );
		iv++;
	}
	free( v );
}



// Destroy all records
void free_records ( record_t ***v ) {
	record_t ***ov = v;
	while ( *ov ) {
		free_inner_records( *ov );
		ov++;
	}
	free( v );
}



// Generate a list of records
record_t *** generate_records ( char *buf, char *del, char **headers, char *err ) {
	record_t **columns = NULL, ***rows = NULL;
	int ol = 0, il = 0, hindex = 0, line = 2;
	zWalker p = { 0 };

	//Find the first '\n' and skip it
	while ( *(buf++) != '\n' );

	//Now allocate for values
	while ( strwalk( &p, buf, del ) ) {
		//fprintf( stderr, "%d, %d\n", p.chr, *p.ptr );
		if ( p.chr == '\r' ) {	
			line++;
			continue;
		}
		else if ( hindex > hlen ) {
			( !rows ) ? free_inner_records( columns ) : free_records( rows );
			snprintf( err, ERRLEN - 1,
				"There may be more columns than headers in this file. "
				"(hindex = %d, hlen = %d)\n", hindex, hlen );
			return NULL;
		}

		//TODO: There are other places to pay more attention to, but
		//this whole block just looks off to me
		record_t *v = malloc( sizeof( record_t ) );
		v->k = headers[ hindex ];
		v->v = malloc( p.size + 1 );
		memset( v->v, 0, p.size + 1 );
		extract_value_from_column( &buf[ p.pos ], ( char ** )&v->v, p.size - 1 );	

		// Do the typecheck ehre
		if ( typesafe ) {
			v->exptype = *(gtypes[ hindex ]);
			v->type = get_type( ( char * )v->v, v->exptype );

			if ( typesafe && v->exptype != v->type ) {
				( !rows ) ? free_inner_records( columns ) : free_records( rows );
				sqldef_t *col = &coldefs[ dbengine ];
				snprintf( err, ERRLEN - 1, 
					"Type check for value at column '%s', line %d failed: %s != %s ( '%s' )\n", 
					v->k, line, col->coldefs[ (int)v->exptype ], col->coldefs[ (int)v->type ],
					v->v );
				return NULL;
			}
		}

		add_item( &columns, v, record_t *, &il );
		hindex++;

		if ( p.chr == '\n' ) {
			add_item( &rows, columns, record_t **, &ol );	
			columns = NULL, il = 0, hindex = 0, line++;
		}
	}

	//This ensures that the final row is added to list...
	//TODO: It'd be nice to have this within the abrowse loop.
	add_item( &rows, columns, record_t **, &ol );	
	return rows;
}



// Output a set of records to whatever output format the user has specified.
void output_records ( record_t ***ov, FILE *output, stream_t stream ) {
	int odv = 0;
	while ( *ov ) {
		//record_t **bv = *ov;

		//Prefix
		if ( prefix ) {
			fprintf( output, "%s", prefix );
		}

		if ( stream == STREAM_PRINTF )
			;//p_default( 0, ib->k, ib->v );
		else if ( stream == STREAM_XML )
			( prefix && newline ) ? fprintf( output, "%c", '\n' ) : 0;
		else if ( stream == STREAM_COMMA )
			;//fprintf( output, "" );
		else if ( stream == STREAM_CSTRUCT )
			fprintf( output, "%s{", odv ? "," : " "  );
		else if ( stream == STREAM_CARRAY )
			fprintf( output, "%s{ ", odv ? "," : " "  ); 
		else if ( stream == STREAM_JSON ) {
		#if 1
			fprintf( output, "%c{%s", odv ? ',' : ' ', newline ? "\n" : "" );
		#else
			if ( prefix && newline )
				fprintf( output, "%c{\n", odv ? ',' : ' ' );
			else if ( prefix ) 
				;
			else {
				fprintf( output, "%c{%s", odv ? ',' : ' ', newline ? "\n" : "" );
			}
		#endif
		}
		else if ( stream == STREAM_SQL ) {
			record_t **hv = *ov;
			int a = 0;
			fprintf( output, "INSERT INTO %s ( ", root );
			while ( *hv && (*hv)->k ) {
				fprintf( output, "%s%s", ( a++ ) ? "," : "", (*hv)->k );
				hv++;	
			}
	
			//The VAST majority of engines will be handle specifying the column names 
			fprintf( output, " ) VALUES ( " );
		}
#if 0
		//The body
		adv = 1;
		while ( *bv && (*bv)->k ) {
			record_t *ib = *bv;
			//Sadly, you'll have to use a buffer...
			//Or, use hlen, since that should contain a count of columns...
			if ( stream == STREAM_PRINTF )
				p_default( 0, ib );
			else if ( stream == STREAM_XML )
				p_xml( 1, ib );
			else if ( stream == STREAM_CSTRUCT )
				p_cstruct( 1, ib );
			else if ( stream == STREAM_CARRAY )
				p_carray( 0, ib );
			else if ( stream == STREAM_COMMA )
				p_comma( 1, ib );
			else if ( stream == STREAM_SQL )
				p_sql( 0, ib );
			else if ( stream == STREAM_JSON ) {
				p_json( 1, ib );
			}
			adv = 0, bv++;
		}
#endif

		//Built-in suffix...
		if ( stream == STREAM_PRINTF )
			;//p_default( 0, ib->k, ib->v );
		else if ( stream == STREAM_XML )
			;//p_xml( 0, ib->k, ib->v );
		else if ( stream == STREAM_COMMA )
			;//fprintf( output, " },\n" );
		else if ( stream == STREAM_CSTRUCT )
			fprintf( output, "}" );
		else if ( stream == STREAM_CARRAY )
			fprintf( output, " }" );
		else if ( stream == STREAM_SQL )
			fprintf( output, " );" );
		else if ( stream == STREAM_JSON ) {
			//wipe the last ','
			fprintf( output, "}" );
			//fprintf( output, "," );
		}

		//Suffix
		if ( suffix ) {
			fprintf( output, "%s", suffix );
		}

		if ( newline ) {
			fprintf( output, NEWLINE );
		}
			
		ov++, odv++;
	}
}


function_t f[] = {
	[STREAM_PRINTF ] = { "printf", NULL, NULL, 0, p_default, NULL, NULL, NULL },
	[STREAM_XML    ] = { "xml", NULL, NULL, 0, p_xml, NULL, NULL, NULL },
	[STREAM_COMMA  ] = { "comma", NULL, NULL, 1, p_comma, NULL, NULL, NULL },
	[STREAM_CSTRUCT] = { "c-struct", NULL, "}", 1, p_cstruct, NULL, NULL, NULL },
	[STREAM_CARRAY ] = { "c-array", NULL, " }", 1, p_carray, NULL, NULL, NULL },
	[STREAM_SQL    ] = { "sql", NULL, " );", 0, p_sql, NULL, NULL, NULL },
	[STREAM_JSON   ] = { "json", NULL, "}", 0, p_json, NULL, NULL, NULL },
#ifdef BPGSQL_H
	[STREAM_PGSQL  ] = { "postgres", NULL, "}", 0, p_pgsql, NULL, NULL, NULL },
#endif
#ifdef BMYSQL_H
	[STREAM_MYSQL  ] = { "mysql", NULL, "}", 0, p_mysql, NULL, NULL, NULL },
#endif
#if 0
	[STREAM_JAVA] = { "json" NULL, "}", 0, p_json, NULL, NULL, NULL },
#endif
};



//check for a valid stream
int check_for_valid_stream ( char *st ) {
	for ( int i = 0; i < sizeof(f) / sizeof(function_t); i++ ) {
		if ( strcasecmp( st, f[i].type ) == 0 ) return i;
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
	zWalker p = { 0 };

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




// Generate some headers from some kind of data structure
int headers_f ( const char *file, const char *delim ) {
	char **h = NULL; 
	char *buf = NULL; 
	int buflen = 0;
	char err[ ERRLEN ] = { 0 };
	char del[ 8 ] = { '\r', '\n', 0, 0, 0, 0, 0, 0 };
	char **headers = NULL;
	//schema_t **hxlist = NULL;

	//Create the new delimiter string.
	memcpy( &del[ strlen( del ) ], delim, strlen(delim) );

	if ( !( buf = (char *)read_file( file, &buflen, err, sizeof( err ) ) ) ) {
		return nerr( "%s", err );
	}

#if 1
	if ( !( h = headers = generate_headers( buf, del ) ) ) {
		free( buf );
		return nerr( "Failed to generate headers." );
	}

	while ( h && *h ) {
		fprintf( stdout, "%s\n", *h );
		free( *h );
		h++;
	}
#else
	if ( !( hxlist = generate_headers_from_text( buf, del ) ) ) {

	}
#endif

	//TODO: Free the header list
	free( headers );
	free( buf );
	return 1;
}


#if 0 
char mysql_coldefs[10][10] = {
	{ "NULL", "TEXT", "TEXT", "INTEGER", "INTEGER", "TEXT" } 
	NULL,		
} };
#endif


// Generate a schema according to a specific type
int schema_f ( const char *file, const char *delim ) {
	char **h = NULL, **headers = NULL;
	char *buf = NULL; 
	char *str = NULL;
	int buflen = 0;
	char err[ ERRLEN ] = { 0 };
	char del[ 8 ] = { '\r', '\n', 0, 0, 0, 0, 0, 0 };
	type_t **types = NULL;
	
	// Set this to keep commas at the beginning
	adv = 1;

	//Create the new delimiter string.
	memcpy( &del[ strlen( del ) ], delim, strlen(delim) );

	if ( !( buf = (char *)read_file( file, &buflen, err, sizeof( err ) ) ) ) {
		return nerr( "%s", err );
	}

	if ( !( h = headers = generate_headers( buf, del ) ) ) {
		free( buf );
		return nerr( "Failed to generate headers." );
	}

	if ( !( str = extract_row( buf, buflen ) ) ) {
		free( buf );
		return nerr( "Failed to extract first row..." );
	}

	types = get_types( str, del );

	// Root should have been specified
	// If not, I don't know...
	fprintf( stdout, "CREATE TABLE %s (\n", root );

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

	// Add the rest of the rows
	while ( h && *h ) {
		sqldef_t *col = &coldefs[ dbengine ];
		const char *type = col->coldefs[ (int)**types ];
		//fprintf( stderr, "%s -> %s\n", *j, type );
		( adv == 1 ) ? 0 : fprintf( stdout, "," );
		fprintf( stdout, "%s %s\n", *h, type );
		free( *h ), free( *types );
		h++, types++, adv = 0;
	}

	fprintf( stdout, ");\n" );

	//TODO: Free the header list
	free( str );
	free( headers );
	free( buf );
	return 1;
}



// Generate a schema according to a specific type
int struct_f ( const char *file, const char *delim ) {
	char **h = NULL, **headers = NULL;
	char *buf = NULL, *str = NULL;
	int buflen = 0;
	char err[ ERRLEN ] = { 0 };
	char del[ 8 ] = { '\r', '\n', 0, 0, 0, 0, 0, 0 };
	type_t **types = NULL;
	
	// Set this to keep commas at the beginning
	adv = 1;

	//Create the new delimiter string.
	memcpy( &del[ strlen( del ) ], delim, strlen(delim) );

	if ( !( buf = (char *)read_file( file, &buflen, err, sizeof( err ) ) ) ) {
		return nerr( "%s", err );
	}

	if ( !( h = headers = generate_headers( buf, del ) ) ) {
		free( buf );
		return nerr( "Failed to generate headers." );
	}

	//
	if ( !( str = extract_row( buf, buflen ) ) ) {
		free( buf );
		return nerr( "Failed to extract first row..." );
	}

	// TODO: Catch this failure should it happen
	types = get_types( str, del );

	// Root should have been specified
	// If not, I don't know...
	if ( !strcmp( root, "root" ) ) {
		return nerr( "Class or struct name unspecified when generating structure\n" );
	}

#if 0
	// Add an ID if we specify it? (unsure how to do this)
	if ( want_id ) {
		fprintf( stdout, "%s INTEGER PRIMARY KEY AUTOINCREMENT\n", id_name );
		adv = 0;
	}
#endif

	//
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
	while ( h && *h ) {
		char *type = defs[ (int)**types ];
		//fprintf( stderr, "%s -> %s\n", *j, type );
		fprintf( stdout, "\t%s %s;\n", type, *h );
		free( *h ), free( *types );
		h++, types++, adv = 0;
	}

	fprintf( stdout, "};\n" );

	//TODO: Free the header list
	free( str );
	free( headers );
	free( buf );
	return 1;
}



#if 0
// Generate a list of types
int types_f ( const char *file, const char *delim ) {
	char **h = NULL, **headers = NULL;
	char *buf = NULL; 
	int buflen = 0;
	char err[ ERRLEN ] = { 0 };
	char del[ 8 ] = { '\r', '\n', 0, 0, 0, 0, 0, 0 };

	//Create the new delimiter string.
	memcpy( &del[ strlen( del ) ], delim, strlen(delim) );
	
	//...
	if ( !( buf = (char *)read_file( file, &buflen, err, sizeof( err ) ) ) ) {
		return nerr( "%s", err );
	}

	// Move up the string to the next line
	char *start = memchr( buf, '\n', buflen );  
return 0;
	while ( strwalk( &p, buf, del ) ) {
		char *t = ( !p.size ) ? no_header : copy_and_snakecase( &buf[ p.pos ], p.size - 1 );
		add_item( &headers, t, char *, &hlen );
		if ( p.chr == '\n' || p.chr == '\r' ) break;
	}
	while ( h && *h ) {
		fprintf( stdout, "%s\n", *h );
		free( *h );
		h++;
	}

	//TODO: Free the header list
	free( headers );
	free( buf );
	return 1;
}
#endif



//convert CSV or similar to another format
int convert_f ( const char *file, const char *delim, stream_t stream ) {
	char *buf = NULL, **headers = NULL;
 	char err[ ERRLEN ] = { 0 }, del[8] = { '\r', '\n', 0, 0, 0, 0, 0, 0 };
	record_t ***ov = NULL;
	FILE *output = stdout;
	int buflen = 0;

	if ( strlen( delim ) > 7 ) {
		return nerr( "delimiter too big (exceeds max of 7 chars), stopping...\n" );
	}

	//Create the new delimiter string.
	memcpy( &del[ strlen( del ) ], delim, strlen(delim) );

	if ( !( buf = (char *)read_file( file, &buflen, err, sizeof( err ) ) ) ) {
		return nerr( "%s\n", err );
	}

	if ( !( headers = generate_headers( buf, del ) ) ) {
		free( buf );
		return nerr( "%s\n", "Failed to generate headers." );
	}

//fprintf( stderr, "DEL: %s\n", delim );
	//
	if ( typesafe ) {
		char *str = extract_row( buf, buflen );
//fprintf( stderr, "STR: %s\n", str );
		type_t ** types = get_types( str, del );
		gtypes = types;
		free( str );
	}

	if ( !( ov = generate_records( buf, del, headers, err ) ) ) {
		free_headers( headers );
		free( buf );
		return nerr( "%s\n", err );
	}

	//Output the string or render, should probably be done with function pointers anyway...
	if ( output_file && !( output = fopen( output_file, "w" ) ) ) {
		snprintf( err, sizeof( err ), "Failed to open file '%s' for writing: %s", output_file, strerror(errno) );
		return nerr( "%s\n", err );
	}

	output_records( ov, output, stream );

	if ( output_file && fclose( output ) ) {
		snprintf( err, sizeof( err ), "Failed to close file '%s': %s", output_file, strerror(errno) );
		return nerr( "%s\n", err );
	}

	free( buf );
	free_records( ov );
	free_headers( headers );
	return 1;
}


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
int parse_dsn_info( dsn_t *conn ) {
	
	// Define	
	//dbtype_t type = DB_NONE;
	int lp = -1;
	char port[ 6 ] = {0};
	char *dsn = NULL;
	unsigned int dsnlen = 0;


	// Cut out silly stuff
	if ( !( dsn = conn->connstr ) || ( dsnlen = strlen( dsn ) ) < 1 ) {
		fprintf( stderr, "Input source is either unspecified or zero-length...\n" );
		return 0;
	}

	
	// Get the database type
	if ( strlen( dsn ) > 8 && !memcmp( dsn, "mysql://", 8 ) ) 
		conn->type = DB_MYSQL, conn->port = 3306, dsn += 8, dsnlen -= 8;
	else if ( strlen( dsn ) > 13 &&!memcmp( dsn, "postgresql://", strlen( "postgresql://" ) ) ) 
		conn->type = DB_POSTGRESQL, conn->port = 5432, dsn += 13, dsnlen -= 13;
	else if ( strlen( dsn ) > 11 &&!memcmp( dsn, "postgres://", strlen( "postgres://" ) ) ) 
		conn->type = DB_POSTGRESQL, conn->port = 5432, dsn += 11, dsnlen -= 11;
	else if ( strlen( dsn ) > 10 &&!memcmp( dsn, "sqlite3://", strlen( "sqlite3://" ) ) )
		conn->type = DB_SQLITE, conn->port = 0, dsn += 10, dsnlen -= 10;
#if 0
	else if ( !memcmp( dsn, "file://", strlen( "file://" ) ) )
		conn->type = DB_NONE, conn->port = 0, conn->connstr += 7, dsnlen -= 7;
#endif
	else {
		if ( strlen( dsn ) > 7 && !memcmp( dsn, "file://", 7 ) )
			conn->connstr += 7, dsnlen -= 7;
		else if ( strlen( dsn ) == 1 && *dsn == '-' ) {
			conn->connstr = "/dev/stdin";
		}
		// TODO: Check valid extensions
		conn->type = DB_NONE; 
		conn->port = 0; 
		return 1;
	}	


	// Handle the different string types as best you can
	// A) localhost, localhost:port doesn't make much sense...
	if ( !memchr( dsn, ':', dsnlen ) && !memchr( dsn, '@', dsnlen ) && !memchr( dsn, '/', dsnlen ) ) {
		lp = safecpypos( dsn, conn->hostname, NULL, sizeof( conn->hostname ) ); 	
	} 
	// C) localhost/database, localhost:port/database
	else if ( memchr( dsn, '/', dsnlen ) && !memchr( dsn, '@', dsnlen ) ) {
		// Copy the port
		if ( memchr( dsn, ':', dsnlen ) ) {
			lp = safecpypos( dsn, conn->hostname, ":", 256 );
			dsn += lp + 1;

			// Handle port
			lp = safecpypos( dsn, port, "/", 256 );
			for ( char *p = port; *p; p++ ) {
				if ( !memchr( "0123456789", *p, 10 ) ) {
					fprintf( stderr, "Port number invalid in DSN...\n" );
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
		char a[ 2048 ], b[ 2048 ], *aa = NULL, *bb = NULL; 
		memset( a, 0, sizeof( a ) );
		memset( b, 0, sizeof( b ) );
		aa = a, bb = b;

		// Stop if we get something weird
		if ( memchrocc( dsn, '@', dsnlen ) > 1 ) {
			fprintf( stderr, "DSN is invalid... ( '@' character should only appear once )\n" );
			return 0;
		}

		// Split the two halves of a proper DSN
		if ( ( p = memchrat( dsn, '@', dsnlen ) ) < sizeof( a ) ) {
			memcpy( a, dsn, p );
		}

		//
		if ( ( dsnlen - ( p + 1 ) ) < sizeof( b ) ) {
			memcpy( b, &dsn[ p + 1 ], dsnlen - ( p + 1 ) );
		}

		// Stop if there is no slash, b/c this means that there is no db
		if ( memchrocc( b, '/', sizeof( b ) ) < 1 ) {
			fprintf( stderr, "No database specified in DSN...\n" );
			return 0;
		}

		// Get user and optional password
		if ( memchr( a, ':', sizeof( a ) ) ) {
			p = safecpypos( aa, conn->username, ":", sizeof( conn->username ) ); 
			if ( !p ) {
				fprintf( stderr, "Username in DSN is too large for buffer (max %ld chars)...\n", sizeof( conn->username ) );
				return 0;
			}
			aa += p + 1;

			if ( !safecpypos( aa, conn->password, NULL, sizeof( conn->password ) ) ) {
				fprintf( stderr, "Password in DSN is too large for buffer (max %ld chars)...\n", sizeof( conn->password ) );
				return 0;
			}
		}
		else {
			if ( !safecpypos( a, conn->username, NULL, sizeof( conn->username ) ) ) {
				fprintf( stderr, "Username in DSN is too large for buffer (max %ld chars)...\n", sizeof( conn->username ) );
				return 0;
			}
		}

		#if 0
		// Disable options for now
		if ( memchr( b, '?', sizeof( b ) ) ) {
			fprintf( stderr, "UNFORTUNATELY, ADVANCED CONNECTION OPTIONS ARE NOT CURRENTLY SUPPORTED!...\n" );
			exit( 0 );
			return 0;
		}
		#endif

		// Get hostname and all of the other stuff if it's there
		// hostname, port, and / (and maybe something else)
		if ( memchr( bb, ':', strlen( bb ) ) ) {
			
			// copy hostname first
			p = safecpypos( bb, conn->hostname, ":", sizeof( conn->hostname ) ), bb += p + 1;

			// hostname should always exist
			if ( !( p = safecpypos( bb, port, "/", sizeof( port ) ) ) ) {
				fprintf( stderr, "Port number given in DSN is invalid...\n" );
				return 0;
			}

			// If this is not all 
 			bb += p + 1;
			if ( ( conn->port = safecpynumeric( port ) ) == -1 ) {
				fprintf( stderr, "Port number given in DSN (%s) is not numeric...\n", port );
				return 0;
			}

			if ( conn->port > 65535 ) {
				fprintf( stderr, "Port number given in DSN (%d) is invalid...\n", conn->port );
				return 0;
			}

			// Copy db or both db and table name
			if ( !memchr( bb, '.', strlen( bb ) ) ) {
				char *end = memchr( bb, '?', strlen( bb ) ) ? "?" : NULL;
				if ( !safecpypos( bb, conn->dbname, end, sizeof( conn->dbname ) ) ) {
					fprintf( stderr, "Database name is too large for buffer...\n" );
					return 0;
				}

				if ( end ) {
					conn->connoptions = end;
				}
			}
			else {
				char *end = memchr( bb, '?', strlen( bb ) ) ? "?" : NULL;

				if ( !( p = safecpypos( bb, conn->dbname, ".", sizeof( conn->dbname ) ) ) ) {
					fprintf( stderr, "Database name is too large for buffer...\n" );
					return 0;
				}
				
				bb += p + 1;

				if ( !safecpypos( bb, conn->tablename, end, sizeof( conn->tablename ) ) ) {
					fprintf( stderr, "Table name is too large for buffer...\n" );
					return 0;
				}

				if ( end ) {
					conn->connoptions = end;
				}
			}
		}

		//
		else if ( memchr( bb, '/', strlen( bb ) ) ) {
			if ( !( p = safecpypos( bb, conn->hostname, "/", sizeof( conn->hostname ) ) ) ) {
				fprintf( stderr, "Hostname too large for buffer (max %ld chars)...\n", sizeof( conn->hostname ) );
				return 0;
			}

			bb += p + 1;
			
			// Copy db or both db and table name
			if ( !memchr( bb, '.', strlen( bb ) ) ) {
				if ( !safecpypos( bb, conn->dbname, NULL, sizeof( conn->dbname ) ) ) {
					fprintf( stderr, "Database name is too large for buffer...\n" );
					return 0;
				}
			}
			else {
				if ( !( p = safecpypos( bb, conn->dbname, ".", sizeof( conn->dbname ) ) ) ) {
					fprintf( stderr, "Database name is too large for buffer...\n" );
					return 0;
				}
				
				bb += p + 1;

				if ( !safecpypos( bb, conn->tablename, NULL, sizeof( conn->tablename ) ) ) {
					fprintf( stderr, "Table name is too large for buffer...\n" );
					return 0;
				}
			}
		}
	}
		
	return 1;
}



// the idea would be to roll through and stream to the same format
int mysql_exec( dsn_t *db, const char *tb, const char *query_optl ) {
/*
+--------------------+
| Database           |
+--------------------+
| itdb_local         |
| schdb              |
+--------------------+
+-------------------------+
| Tables_in_itdb_local    |
+-------------------------+
| ncat_bir_definitions    |
| ncat_bir_specifications |
+-------------------------+
*/
	MYSQL *conn = NULL; 
	MYSQL_RES *res = NULL; 
	record_t ***rows = NULL;
	char **headers = NULL;	
	char *query = NULL;
	int rowlen = 0;
	int fcount = 0;
	int hlen = 0;
	int blimit = 1000;
	FILE *output = stdout;
	char *tablename = NULL;

	// This is an error, I feel like we should never get here
	if ( !db ) {
		fprintf( stderr, "No DSN specified for source data...\n" );
		return 0;
	}

	// Stop if no table name was specified
	if ( !*db->tablename && ( !tb || strlen( tb ) < 1 )  ) {
		fprintf( stderr, "No table name specified for source data...\n" );
		return 0;
	}

	// Choose a table name
	if ( tb ) 
		tablename = (char *)tb;
	else {
		tablename = db->tablename;
	}

	// connect to the database
	if ( !( conn = mysql_init( 0 ) ) ) {
		fprintf( stderr, "Couldn't initialize MySQL connection: %s\n", mysql_error( conn ) );
		return 0;
	}

	if ( !mysql_real_connect( conn, db->hostname, db->username, db->password, db->dbname, db->port, NULL, 0 ) ) {
		fprintf( stderr, "Couldn't connect to server: %s\n", mysql_error( conn ) );
		exit( 1 );
		return 0;
	}

	//fprintf( stderr, "Connection to MySQL was successful: (%p)\n", (void *)conn );
	if ( query_optl )
		query = (char *)query_optl;	
	else {
		query = malloc( 1024 );
		memset( query, 0, 1024 );
		snprintf( query, 1023, "SELECT * FROM %s", tablename );
	}
	
	// Execute whatever query
	if ( mysql_query( conn, query ) > 0 ) {
		fprintf( stderr, "Failed to run query against selected db and table: %s\n", mysql_error( conn ) );
		mysql_close( conn );
		return 0;
	}

	// Use the result
	if ( !( res = mysql_store_result( conn ) ) ) {
		fprintf( stderr, "Failed to store results set from last query: %s\n", mysql_error( conn ) );
		mysql_close( conn );
		return 0;
	}

	// Get the field count ONCE
	fcount = mysql_field_count( conn );
int want_schema = 1;

	// Create the table for PG
	if ( want_schema ) {
		fprintf( stdout, "CREATE TABLE IF NOT EXISTS %s (\n", tablename );
	}

	schema_t **slist = NULL;
	int slen = 0;

	// Get whatever row names and create the headers
	for ( int i = 0; i < fcount; i++ ) {
		MYSQL_FIELD *f = mysql_fetch_field_direct( res, i ); 	
		//fprintf( stdout, "%s (len: %ld, charset: %d)\n", f->name, f->length, f->charsetnr ); // list of types is there too
		// type -> mysql_com.h and enum_field_types will spell this out
		// char *name = strdup( f->name );
		add_item( &headers, f->name, char *, &hlen ); 

		if ( want_schema ) {
			// All of the tables should use this
			schema_t *st = malloc( sizeof( schema_t ) );
			memset( st, 0, sizeof( schema_t ) );
			snprintf( st->label, sizeof( st->label ), "%s", f->name );	

		// TODO: Optionally, you can create a schema using the info here if that's the idea
			// Generate each row
			type_t type = 0; 
			if ( f->type == MYSQL_TYPE_NULL )
				type = T_NULL;
			else if ( f->type == MYSQL_TYPE_BIT )
				type = T_BOOLEAN;
			else if ( CHECK_MYSQL_TYPE( f->type, _mysql_strings ) )
				type = T_STRING;
			else if ( CHECK_MYSQL_TYPE( f->type, _mysql_chars ) )
				type = T_CHAR;
			else if ( CHECK_MYSQL_TYPE( f->type, _mysql_integers ) )
				type = T_INTEGER;
			else if ( CHECK_MYSQL_TYPE( f->type, _mysql_doubles ) )
				type = T_DOUBLE;
			else if ( CHECK_MYSQL_TYPE( f->type, _mysql_blobs ) || CHECK_MYSQL_TYPE( f->type, _mysql_dates ) ) {
				// Unfortunately, I don't support blobs yet.
				// There are so many cool ways to handle this, but we're not there yet...	
				//fprintf( stderr, "UNFORTUNATELY, BLOBS ARE NOT YET SUPPORTED!\n" );
				if ( rows ) {
					free_records( rows ); 
				}
				mysql_free_result( res );
				mysql_close( conn );
				free( headers );
				return 0;
			}

fprintf( stdout, "%s %s,", f->name, mysql_coldefs[ (int)type ] );		
#if 0
fprintf( stdout, "%s %s,", f->name, mysql_coldefs[ (int)type ] );		
fprintf( stdout, "cat: %s, ", f->catalog );
fprintf( stdout, "def: %s, ", f->def);
fprintf( stdout, "flags: %d\n", IS_PRI_KEY( f->flags ) );
#endif

			if ( IS_PRI_KEY( f->flags ) ) {
				st->primary = 1;
			}

			add_item( &slist, st, schema_t *, &slen );
		}

	}

	if ( want_schema ) {
		// You can loop through the created headers and figure out how to set the primary key (since mysql can't do it like others) 
		fprintf( stdout, ");\n" );
	}
exit(0);
	// If a limit was specified, we'll figure out how to divide that up here
	// get total rows
	my_ulonglong rowcount = mysql_num_rows( res );
	// divide by approximate buffer limit (not sure how to make this make sense)
	int loopcount = ( rowcount / blimit < 1 ) ? 1 : rowcount / blimit;
	if ( rowcount % blimit && loopcount > 1 ) {
		loopcount++;
	}

	// loop through that
	//fprintf( stderr, "fc: %d, lc: %d, rc: %lld, x: %lld\n", fcount, loopcount, rowcount, rowcount % blimit );

	// You can stream some at a time to save memory, so an -L, --limit optino will be helpful
#if 1
	for ( int lc = 0, rc = 0; lc < loopcount; lc++ ) {
		for ( int i = 0; i < blimit && rc < rowcount ; i++, rc++ ) {
		
			// Define all this	
			int collen = 0;
			record_t **columns = NULL;
			MYSQL_ROW row = mysql_fetch_row( res );

			for ( int x = 0; x < fcount; x++ ) {
				// Optionally print the row	
				//fprintf( stdout, "%s: %s\n", headers[ x ], *row );

				// Define a new record
				record_t *cell = malloc( sizeof( record_t ) );
				memset( cell, 0, sizeof( record_t ) );
				cell->k = headers[ x ];
				cell->v = (unsigned char *)strdup( *row++ );

				// Check type	
				if ( typesafe ) {
					MYSQL_FIELD *f = mysql_fetch_field_direct( res, x ); 	
					if ( f->type == MYSQL_TYPE_NULL )
						cell->type = T_NULL, DPRINTF( "Type of %s was NULL\n", f->name );
					else if ( f->type == MYSQL_TYPE_BIT )
						cell->type = T_BOOLEAN, DPRINTF( "Type of %s was BOOLEAN\n", f->name );
					else if ( CHECK_MYSQL_TYPE( f->type, _mysql_strings ) )
						cell->type = T_STRING, DPRINTF( "Type of %s was STRING\n", f->name );
					else if ( CHECK_MYSQL_TYPE( f->type, _mysql_chars ) )
						cell->type = T_CHAR, DPRINTF( "Type of %s was CHAR\n", f->name );
					else if ( CHECK_MYSQL_TYPE( f->type, _mysql_integers ) )
						cell->type = T_INTEGER, DPRINTF( "Type of %s was INTEGER\n", f->name );
					else if ( CHECK_MYSQL_TYPE( f->type, _mysql_doubles ) )
						cell->type = T_DOUBLE, DPRINTF( "Type of %s was DOUBLE\n", f->name );
					else if ( CHECK_MYSQL_TYPE( f->type, _mysql_blobs ) || CHECK_MYSQL_TYPE( f->type, _mysql_dates ) ) {
						// Unfortunately, I don't support blobs yet.
						// There are so many cool ways to handle this, but we're not there yet...	
						fprintf( stderr, "UNFORTUNATELY, BLOBS ARE NOT YET SUPPORTED!\n" );
						if ( rows ) {
							free_records( rows ); 
						}
						mysql_free_result( res );
						mysql_close( conn );
						free( headers );
						return 0;
					}
				}

				// Just figure out types and this is solved...
				// Type can get written too, but might not be necessary
				// It will be necessary if trying to create an insert out of it
				add_item( &columns, cell, record_t *, &collen );  
			}

			add_item( &rows, columns, record_t **, &rowlen );
			//fprintf( stdout, "\n" );
		}

		// You can print each set with output_records(...) 
		output_records( rows, output, STREAM_SQL );	
	}
#endif

	// Destroy everything
	mysql_free_result( res );
	free_records( rows );
	free( headers );
	if ( !query_optl ) {
		free( query );
	} 
	mysql_close( conn );
	return 1;
}



int postgresql_exec( dsn_t *db, const char *table, const char *qopt ) {
	char *start = NULL;
	char *tablename = NULL;
	char query[ 1024 ]; 
	int fcount = 0;
	int rcount = 0;
	PGconn *conn = NULL;
	PGresult *res = NULL;

	// Do init stuff
	memset( query, 0, 1024 );

	// DSN should always be specified
	if ( !db ) {
		fprintf( stderr, "No DSN specified for source data...\n" );
		return 0;
	}

	// Stop if no table name was specified
	if ( !(*db->tablename) && ( !table || strlen( table ) < 1 )  ) {
		fprintf( stderr, "No table name specified for source data...\n" );
		return 0;
	}

	// Choose a table name
	if ( table ) 
		tablename = (char *)table;
	else {
		tablename = db->tablename;
	}

	// It is possible that regular connections don't work, so either
	// make the special one or get rid of the table notation
	if ( db->connstr && ( start = memchr( db->connstr, '@', strlen( db->connstr ) ) ) ) {
		// If the table has been specified, get rid of everything before the '?'
		char *tstart = memchr( start, '.', strlen( start ) );
		char *qstart = memchr( start, '?', strlen( start ) );

	#if 1
		if ( tstart ) {
			// Terminate the string and disregard the rest of the string for initializing PG
			if ( !qstart ) {
				*tstart = '\0';
			}
			else {
				int qlen = strlen( qstart );
				memmove( tstart, qstart, qlen ); 
				*(tstart + qlen) = '\0';
			}

			//fprintf( stderr, "CONN = %s\n", db->connstr );
		}
	#endif
	}

	// Create a connection
	conn = PQconnectdb( db->connstr );
	fprintf( stderr, "pg conn = %p\n", (void *)conn );

	// Check that it was successful
	if ( PQstatus( conn ) != CONNECTION_OK ) {
		fprintf( stderr, "No DSN specified for source data...\n" );
		PQfinish( conn );
		return 0;
	}

	#if 0
	// Set a secure search path?
	res = PQexec( conn, "SELECT pg_catalog.set_config( 'search_path', '', false )" );
	if ( PQresultStatus( res ) != PGRES_TUPLES_OK ) {
		fprintf( stderr, "SET failed: %s...\n", PQerrorMessage( conn ) );
		PQclear( res );
		PQfinish( conn );
		return 0;
	}

	// Clear any potential leaks from an open connection
	PQclear( res );
	#endif

	// Format the query
	if ( qopt ) 
		snprintf( query, sizeof( query ) - 1, "%s", qopt );
	else {
		snprintf( query, sizeof( query ) - 1, "SELECT * FROM %s", tablename );
	}
	
	// Run the querys
fprintf(stdout, "%s\n", query );
fprintf(stdout, "%s\n", tablename );
	res = PQexec( conn, query );
	if ( PQresultStatus( res ) != PGRES_COMMAND_OK ) {
		fprintf( stderr, "Failed to run query against selected db and table: '%s'\n", PQerrorMessage( conn ) );
		PQclear( res );
		return 0;
	}

exit(0);

	// Fetch each field
	fcount = PQnfields( res );
	for ( int i = 0; i < fcount; i++ ) {
		char *cname = PQfname( res, i );
fprintf( stdout, "CNAME (%d) = %s, ", i, cname );

		Oid ftype = PQftype( res, i );
fprintf( stdout, "%d\n", ftype );
	}

	// Then fetch each row
	rcount = PQntuples( res );
	for ( int i = 0; i < rcount; i++ ) {
		for ( int ii = 0; ii < fcount; ii++ ) {
			// If it's null?  I don't know what to do.  If it's an int, that's a problem
			// if PQgetisnull( res, i, ii );
			char *cvalue = PQgetvalue( res, i, ii );
			fprintf( stdout, "%s => %s\n", PQfname( res, ii ), cvalue );
		}
	}
		

	// Clear the result set
	PQclear( res );

	// Close the connection
	PQfinish( conn );
	return 1;
}



#ifndef DEBUG_H
 #define print_dsn (A) 0
 #define print_headers(A) 0 
 #define print_stream_type(A) 0
#else
// Dump the DSN
void print_dsn ( dsn_t *conninfo ) {
	printf( "dsn->connstr:   %s\n", conninfo->connstr );	
	printf( "dsn->type:       %d\n", conninfo->type );
	printf( "dsn->username:   '%s'\n", conninfo->username );
	printf( "dsn->password:   '%s'\n", conninfo->password );
	printf( "dsn->hostname:   '%s'\n", conninfo->hostname );
	printf( "dsn->dbname:     '%s'\n", conninfo->dbname );
	printf( "dsn->tablename:  '%s'\n", conninfo->tablename );
	//printf( "dsn->socketpath: '%s'\n", conninfo->socketpath );
	printf( "dsn->port:       %d\n", conninfo->port );
	printf( "dsn->options:    %d\n", conninfo->options );	
	printf( "dsn->conn:       %p\n", conninfo->conn );	
}


// Dump any headers
void print_headers( dsn_t *t ) {
	for ( schema_t **s = t->headers; s && *s; s++ ) {
		printf( "%s %d\n", (*s)->label, (*s)->type );
	}
}

void print_stream_type( stream_t t ) {
	if ( t == STREAM_NONE )
		printf( "STREAM_NONE\n" );
	else if ( t == STREAM_PRINTF )
		printf( "STREAM_PRINTF\n" );
	else if ( t == STREAM_JSON )
		printf( "STREAM_JSON\n" );
	else if ( t == STREAM_XML )
		printf( "STREAM_XML\n" );
	else if ( t == STREAM_SQL )
		printf( "STREAM_SQL\n" );
	else if ( t == STREAM_CSTRUCT )
		printf( "STREAM_CSTRUCT\n" );
	else if ( t == STREAM_COMMA )
		printf( "STREAM_COMMA\n" );
	else if ( t == STREAM_CARRAY )
		printf( "STREAM_CARRAY\n" );
	else if ( t == STREAM_JCLASS )
		printf( "STREAM_JCLASS\n" );
	else if ( t == STREAM_CUSTOM )
		printf( "STREAM_CUSTOM\n" );
	else if ( t == STREAM_PGSQL )
		printf( "STREAM_PGSQL\n" );
	else if ( t == STREAM_MYSQL ) {
		printf( "STREAM_MYSQL\n" );
	}	
}
#endif


void destroy_dsn_headers ( dsn_t *t ) {
	for ( schema_t **s = t->headers; s && *s; s++ ) {
		free( *s );
	}
	free( t->headers );
}



/**
 * create_dsn( dsn_t * )
 * ===================
 *
 * Create a data source.  (A file, database if SQLite3 or
 * a table if a super heavy weight database.)
 * 
 */
int create_dsn ( dsn_t *conn ) {
	// This might be a little harder, unless there is a way to either create a new connection
	// or do something else
	return 1;
}



/**
 * open_dsn( dsn_t * )
 * ===================
 *
 * Open and manage a data source.
 * 
 */
int open_dsn ( dsn_t *conn, const char *qopt ) {
	char query[ 2048 ];
	memset( query, 0, sizeof( query ) );

	// Handle opening the database first
	if ( conn->type == DB_NONE ) {
		DPRINTF( "Opening file at %s\n", conn->connstr );
		char err[ 1024 ] = { 0 };

		// TODO: Allocating everything like this is fast, but not efficient on memory
		if ( !( conn->conn = read_file( conn->connstr, &conn->clen, err, sizeof( err ) ) ) ) {
			fprintf( stderr, "%s\n", err );
			return 0;
		}

		return 1;
	}


	// Stop if no table name was specified
	if ( !table && ( !( *conn->tablename ) || strlen( conn->tablename ) < 1 ) ) {
		fprintf( stderr, "No table name specified for source data...\n" );
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
			const char fmt[] = "Couldn't initialize MySQL connection: %s\n";
			fprintf( stderr, fmt, mysql_error( myconn ) );
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
			const char fmt[] = "Couldn't connect to server: %s\n";
			fprintf( stderr, fmt, mysql_error( myconn ) );
			return 0;
		}

		// Execute whatever query
		if ( mysql_query( myconn, query ) > 0 ) {
			const char fmt[] = "Failed to run query against selected db and table: %s\n"; 
			fprintf( stderr, fmt, mysql_error( myconn ) );
			mysql_close( myconn );
			return 0;
		}

		// Use the result
		if ( !( myres = mysql_store_result( myconn ) ) ) {
			const char fmt[] = "Failed to store results set from last query: %s\n"; 
			fprintf( stderr, fmt, mysql_error( myconn ) );
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
		char *start = NULL, *tstart = NULL, *qstart = NULL;

		//
		conn->defs = postgres_coldefs;

		// Fix the connection string if it hasn't been done yet...
		if ( connstr && ( start = memchr( connstr, '@', strlen( connstr ) ) ) ) {
			// If the table has been specified, get rid of everything before the '?'
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

		// Attempt to open the connection and handle errors
		if ( PQstatus( ( pgconn = PQconnectdb( connstr ) ) ) != CONNECTION_OK ) {
			const char fmt[] = "Couldn't initialize PostsreSQL connection: %s\n";
			fprintf( stderr, fmt, PQerrorMessage( pgconn ) );
			PQfinish( pgconn );
			return 0;
		}

		// Run the query
		if ( !( pgres = PQexec( pgconn, query ) ) ) {
			const char fmt[] = "Failed to run query against selected db and table: '%s'\n";
			fprintf( stderr, fmt, PQerrorMessage( pgconn ) );
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



/**
 * headers_from_dsn( dsn_t * )
 * ===========================
 *
 * Get the headers from a data source.
 * 
 */
int headers_from_dsn ( dsn_t *conn ) {

	// Define 
	int fcount = 0;
	char *str = NULL;

	if ( conn->type == DB_SQLITE ) {
		fprintf( stderr, "SQLite3 not done yet.\n" );
		return 0;	
	}

	else if ( conn->type == DB_MYSQL ) {
		fcount = mysql_field_count( conn->conn ); 
		for ( int i = 0; i < fcount; i++ ) {
			MYSQL_FIELD *f = mysql_fetch_field_direct( conn->res, i ); 	
			schema_t *st = malloc( sizeof( schema_t ) );
	
			// Allocate a new schema_t	
			if ( !st || !memset( st, 0, sizeof( schema_t ) ) ) {
				fprintf( stderr, "Memory constraints encountered while building schema" );
				return 0;	
			}
			
			// Get the type of the column
			if ( f->type == MYSQL_TYPE_NULL )
				st->type = T_NULL;
			else if ( f->type == MYSQL_TYPE_BIT )
				st->type = T_BOOLEAN;
			else if ( CHECK_MYSQL_TYPE( f->type, _mysql_strings ) )
				st->type = T_STRING;
			else if ( CHECK_MYSQL_TYPE( f->type, _mysql_chars ) )
				st->type = T_CHAR;
			else if ( CHECK_MYSQL_TYPE( f->type, _mysql_integers ) )
				st->type = T_INTEGER;
			else if ( CHECK_MYSQL_TYPE( f->type, _mysql_doubles ) )
				st->type = T_DOUBLE;
			else if ( CHECK_MYSQL_TYPE( f->type, _mysql_blobs ) || CHECK_MYSQL_TYPE( f->type, _mysql_dates ) ) {
				fprintf( stderr, "SQLite3 not done yet.\n" );
				return 0;	
			#if 0
				// Unfortunately, I don't support blobs yet.
				// There are so many cool ways to handle this, but we're not there yet...	
				//fprintf( stderr, "UNFORTUNATELY, BLOBS ARE NOT YET SUPPORTED!\n" );
				if ( rows ) {
					free_records( rows ); 
				}
				mysql_free_result( res );
				mysql_close( conn );
				free( headers );
				return 0;
			#endif
			}

			// Write the column name
			snprintf( st->label, sizeof( st->label ), "%s", f->name );	
			add_item( &conn->headers, st, schema_t *, &conn->hlen ); 
		}
	}
	else if ( conn->type == DB_POSTGRESQL ) {
		fcount = PQnfields( conn->res );
		for ( int i = 0; i < fcount; i++ ) {
			char *cname = PQfname( conn->res, i );
			Oid ftype = PQftype( conn->res, i );
			schema_t *st = malloc( sizeof( schema_t ) );

			// Allocate a new schema_t	
			if ( !st || !memset( st, 0, sizeof( schema_t ) ) ) {
				fprintf( stderr, "Memory constraints encountered while building schema" );
				return 0;	
			}


fprintf( stdout, "CNAME (%d) = %s, ", i, cname );
fprintf( stdout, "%d\n", ftype );

				//st->type = T_NULL;
#if 1
			// Get the type of the column
			if ( ftype == MYSQL_TYPE_NULL )
				st->type = T_NULL;
			else if ( ftype == MYSQL_TYPE_BIT )
				st->type = T_BOOLEAN;
			else if ( CHECK_MYSQL_TYPE( ftype, _mysql_strings ) )
				st->type = T_STRING;
			else if ( CHECK_MYSQL_TYPE( ftype, _mysql_chars ) )
				st->type = T_CHAR;
			else if ( CHECK_MYSQL_TYPE( ftype, _mysql_integers ) )
				st->type = T_INTEGER;
			else if ( CHECK_MYSQL_TYPE( ftype, _mysql_doubles ) )
				st->type = T_DOUBLE;
			else if ( CHECK_MYSQL_TYPE( ftype, _mysql_blobs ) || CHECK_MYSQL_TYPE( ftype, _mysql_dates ) ) {
				fprintf( stderr, "SQLite3 not done yet.\n" );
				return 0;	
			#if 0
				// Unfortunately, I don't support blobs yet.
				// There are so many cool ways to handle this, but we're not there yet...	
				//fprintf( stderr, "UNFORTUNATELY, BLOBS ARE NOT YET SUPPORTED!\n" );
				if ( rows ) {
					free_records( rows ); 
				}
				mysql_free_result( res );
				mysql_close( conn );
				free( headers );
				return 0;
			#endif
			}
#endif
			snprintf( st->label, sizeof( st->label ), "%s", cname );	
			add_item( &conn->headers, st, schema_t *, &conn->hlen ); 
		}
	}
	else { 
		// TODO: Reimplement this to read line-by-line
		zWalker p;

		// Prepare this structure
		memset( &p, 0, sizeof( zWalker ) );

		// Copy the delimiter to whatever this is
		memcpy( &delset[ strlen( delset ) ], DELIM, strlen( DELIM ) );

		// Walk through and find each column first
		for ( ; strwalk( &p, conn->conn, delset ); ) {

			// Define
			char *t = ( !p.size ) ? no_header : copy( &((char *)conn->conn)[ p.pos ], p.size - 1, case_type );
			schema_t *st = NULL; //malloc( sizeof( schema_t ) );

			// ...	
			if ( !( st = malloc( sizeof( schema_t ) ) ) || !memset( st, 0, sizeof( schema_t ) ) ) {
				//const char fmt[] = "";
				fprintf( stderr, "Memory constraints encountered while building schema from file: %s", conn->connstr );
				return 0;	
			}
		
			snprintf( st->label, sizeof( st->label ) - 1, "%s", t );
			add_item( &conn->headers, st, schema_t *, &conn->hlen );
			if ( p.chr == '\n' || p.chr == '\r' ) break;
		}

		// If we need to maintain some kind of type safety, run this
		if ( typesafe ) {
			if ( !( str = extract_row( conn->conn, conn->clen ) ) ) {
				//const char fmt[] = "";
				fprintf( stderr, "Failed to extract first row from file: %s\n", conn->connstr ); 
				return 0;	
			}

			// You should do a count really quick on the second rows, or possibly even all rows to make sure that the rows match
			int count = memstrocc( str, &delset[2], strlen( str ) ); 

			// If these don't match, we have a problem
			// Increase by one, since the column count should match the occurrence of delimiters + 1
			if ( conn->hlen != ++count ) {
				//const char fmt[] = "";
				fprintf( stderr, "Header count and column count differ in file: %s (%d != %d)\n", 
					conn->connstr,
					conn->hlen,
					count
				); 
				return 0;	
			}

			// -
			memset( &p, 0, sizeof( zWalker ) );

			// -
			for ( int i = 0, len = 0; strwalk( &p, str, delset ); i++ ) {
				char *t = NULL;

				// Handle NULL or empty strings
				if ( !p.size ) {
					(conn->headers[ i ])->type = T_STRING;
					continue;
				}

				// Handle NULL or empty strings
				t = copy( &str[ p.pos ], p.size - 1, 0 );
				if ( ( len = strlen( t ) ) == 0 ) {
					(conn->headers[ i ])->type = T_STRING;
					continue;
				}

				(conn->headers[ i ])->type = get_type( t, T_STRING ); 
			}
		}
	}

	return 1;
}



/**
 * schema_from_dsn( dsn_t * )
 * ===================
 *
 * Close a data source.
 * 
 */
int schema_from_dsn( dsn_t *conn ) {

	// Create the table for whatever the source may be 
	// fprintf( stdout, "DROP TABLE IF EXISTS %s;\n", conn->tablename );
	fprintf( stdout, "CREATE TABLE IF NOT EXISTS %s (\n", conn->tablename );

	// Depends on output chosen
	int len = conn->hlen;
	schema_t **s = conn->headers;
	const char **tdefs = coldefs[ (int)dbengine ].coldefs;
	

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
	for ( ; s && *s; s++, len-- ) {
		//const char fmt[] = "\t%s %s";
		fprintf( stdout, "\t%s %s", (*s)->label, tdefs[ (int)(*s)->type ] );

		// Are we trying to populate or create a MySQL database
		// Does anyone else support this syntax?
		if ( len == 1 && (*s)->primary ) {
		fprintf( stdout, "PRIMARY KEY" );
		}
	
		if ( len > 1 ) {
			fprintf( stdout, "," );
		}
		fprintf( stdout, "\n" );
	}

	// Terminate the schema
	fprintf( stdout, ");\n" );
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
	for ( schema_t **s = conn->headers; s && *s; s++ ) {
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
int prepare_dsn ( dsn_t *conn ) {
	if ( conn->type == DB_NONE ) {
		while ( *((char *)conn->conn++) != '\n' );
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
int records_from_dsn( dsn_t *conn, int count, int offset ) {

	if ( conn->type == DB_NONE ) {
		zWalker *p = malloc( sizeof( zWalker ) );
		memset( p, 0, sizeof( zWalker ) );
		column_t **cols = NULL;
		row_t *row = NULL;

		for ( int line = 0, ci = 0, clen = 0; strwalk( p, conn->conn, delset ); line++ ) {
			//fprintf( stderr, "%d, %d\n", p.chr, *p.ptr );

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
				fprintf( stderr, fmt, ci, conn->hlen, line, conn->connstr );
				return 0;
			}

			// Allocate, prepare and set column values
			if ( !( col = malloc( sizeof( column_t ) ) ) || !memset( col, 0, sizeof( column_t ) ) ) {
				const char fmt[] = "Out of memory when allocating space for value at line %d, column %d: %s";
				fprintf( stderr, fmt, line, ci, strerror( errno ) );
				return 0;
			}

	#if 1
			// Set any remaining column values
			if ( !( col->v = malloc( p->size + 1 ) ) || !memset( col->v, 0, p->size + 1 ) ) {
				const char fmt[] = "Out of memory when allocating space for value at line %d, column %d: %s";
				fprintf( stderr, fmt, line, ci, strerror( errno ) );
				return 0;
			}

			// Set the header ref and actual column value
			extract_value_from_column( &( (char *)conn->conn )[ p->pos ], (char **)&col->v, p->size - 1 );
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
					fprintf( stderr, fmt, col->k, line );
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
					fprintf( stderr, fmt, line, strerror( errno ) );
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

	//This ensures that the final row is added to list...
	//TODO: It'd be nice to have this within the abrowse loop.
	//add_item( &rows, columns, record_t **, &ol );	
	}
	else if ( conn->type == DB_SQLITE ) {
		;
	}
	else if ( conn->type == DB_MYSQL ) {
	#if 0
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
		my_ulonglong rowcount = mysql_num_rows( conn->res );

		// Move through all of the columns
		for ( int i = 0; i < rowcount ; i++ ) {
			MYSQL_ROW r = mysql_fetch_row( conn->res );
			row_t *row = NULL;
			column_t **cols = NULL;

			// Move through each column
			for ( int ci = 0, clen = 0; ci < conn->hlen; ci++ ) {
				// Define
				column_t *col = malloc( sizeof( column_t ) ); 

				// Check for alloc failure
				if ( !col || !memset( col, 0, sizeof( column_t ) ) ) {
					const char fmt[] = "Out of memory when allocating space for column '%s': %s"; 
					fprintf( stderr, fmt, "x", strerror( errno ) );
					return 0;		
				}

				// Set the values
				col->k = conn->headers[ ci ]->label; 
				col->v = (unsigned char *)strdup( *r );
				col->len = strlen( *r ); 
				add_item( &cols, col, column_t *, &clen );  
				r++;
			}

			// Add a new row
			if ( !( row = malloc( sizeof( row_t ) ) ) ) {
				const char fmt[] = "Out of memory when allocating space for new row at cursor: %s"; 
				fprintf( stderr, fmt, strerror( errno ) );
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
	else if ( conn->type == DB_POSTGRESQL )  {
		;
	}

	return 1;
}




/**
 * transform_from_dsn( dsn_t * )
 * ===================
 *
 * "Transform" the records into whatever output format you're looking for.
 * 
 */
int transform_from_dsn( dsn_t *conn, dsn_t *out, stream_t t, int count, int offset ) {

	// Define
	int odv = 0;

	// Loop through all rows
	for ( row_t **row = conn->rows; row && *row; row++ ) {

		//Prefix
		if ( prefix ) {
			fprintf( out->output, "%s", prefix );
		}

		if ( t == STREAM_PRINTF )
			;//p_default( 0, ib->k, ib->v );
		else if ( t == STREAM_XML )
			( prefix && newline ) ? fprintf( out->output, "%c", '\n' ) : 0;
		else if ( t == STREAM_COMMA )
			;//fprintf( out->output, "" );
		else if ( t == STREAM_CSTRUCT )
			fprintf( out->output, "%s{", odv ? "," : " "  );
		else if ( t == STREAM_CARRAY )
			fprintf( out->output, "%s{ ", odv ? "," : " "  ); 
		else if ( t == STREAM_JSON ) {
		#if 1
			fprintf( out->output, "%c{%s", odv ? ',' : ' ', newline ? "\n" : "" );
		#else
			if ( prefix && newline )
				fprintf( out->output, "%c{\n", odv ? ',' : ' ' );
			else if ( prefix ) 
				;
			else {
				fprintf( out->output, "%c{%s", odv ? ',' : ' ', newline ? "\n" : "" );
			}
		#endif
		}
		else if ( t == STREAM_SQL ) {
			int a = 0;	
			// Output the table name
			fprintf( out->output, "INSERT INTO %s ( ", conn->tablename );

			for ( schema_t **x = conn->headers; x && *x; x++ ) {	
				// TODO: use the pointer to detect whether or not we're at the beginning
				fprintf( out->output, "%s%s", ( a++ ) ? "," : "", (*x)->label );
			}
	
			//The VAST majority of engines will be handle specifying the column names 
			fprintf( out->output, " ) VALUES ( " );
		}
		#if 0	
		else {
			//raw sql shouldn't need anything else...
		}
		#endif	

		// Loop through each column
		for ( column_t **col = (*row)->columns; col && *col; col++ ) {
			//Sadly, you'll have to use a buffer...
			//Or, use hlen, since that should contain a count of columns...
			int first = *col == *((*row)->columns); 
#if 1
			if ( t == STREAM_PRINTF )
				fprintf( out->output, "%s => %s%s%s\n", (*col)->k, ld, (*col)->v, rd );
			else if ( t == STREAM_XML )
				fprintf( out->output, "%s<%s>%s</%s>\n", &TAB[9-1], (*col)->k, (*col)->v, (*col)->k );
			else if ( t == STREAM_CARRAY )
				fprintf( out->output, &", \"%s\""[ first ], (*col)->v );
			else if ( t == STREAM_COMMA )
				fprintf( out->output, &",\"%s\""[ first ], (*col)->v );
			else if ( t == STREAM_JSON ) {
				fprintf( out->output, "%s%c\"%s\": \"%s\"\n", &TAB[9-1], first ? ' ' : ',', (*col)->k, (*col)->v );
			}
			else if ( t == STREAM_SQL ) {
				if ( !typesafe ) {
					fprintf( out->output, &",%s%s%s"[ first ], ld, (*col)->v, rd );
				}
#if 1
				else {
					// Should check that they match too
					if ( (*col)->type != T_STRING && (*col)->type != T_CHAR )
						fprintf( out->output, &",%s"[ first ], (*col)->v );
					else {
						fprintf( out->output, &",%s%s%s"[ first ], ld, (*col)->v, rd );
					}
				}
#endif
			}
			else if ( t == STREAM_CSTRUCT ) {
				//p_cstruct( 1, *col );
				//check that it's a number
				char *vv = (char *)(*col)->v;

				//Handle blank values
				if ( !strlen( vv ) ) {
					fprintf( out->output, "%s", &TAB[9-1] );
					fprintf( out->output, &", .%s = \"\"\n"[ first ], (*col)->k );
				}
				else {
					fprintf( out->output, "%s", &TAB[9-1] );
					fprintf( out->output, &", .%s = \"%s\"\n"[ first ], (*col)->k, (*col)->v );
				}
			}
		#ifdef BPGSQL_H
			else if ( t == STREAM_PGSQL ) {
				// Bind and prepare an insert?
			}
		#endif
		#ifdef BMYSQL_H
			else if ( t == STREAM_MYSQL ) {
				// Bind and prepare an insert?

			}
		#endif
		#if 0
			else if ( t == STREAM_SQLITE3 ) {
			}
		#endif
#endif
		}

		//Built-in suffix...
		if ( t == STREAM_PRINTF )
			;//p_default( 0, ib->k, ib->v );
		else if ( t == STREAM_XML )
			;//p_xml( 0, ib->k, ib->v );
		else if ( t == STREAM_COMMA )
			;//fprintf( out->output, " },\n" );
		else if ( t == STREAM_CSTRUCT )
			fprintf( out->output, "}" );
		else if ( t == STREAM_CARRAY )
			fprintf( out->output, " }" );
		else if ( t == STREAM_SQL )
			fprintf( out->output, " );" );
		else if ( t == STREAM_JSON ) {
			//wipe the last ','
			fprintf( out->output, "}" );
			//fprintf( out->output, "," );
		}

		//Suffix
		if ( suffix ) {
			fprintf( out->output, "%s", suffix );
		}

		if ( newline ) {
			fprintf( out->output, NEWLINE );
		}
	
	}

	return 1;
}



/**
 * close_dsn( dsn_t * )
 * ===================
 *
 * Close a data source.
 * 
 */
int close_dsn( dsn_t *conn ) {

	DPRINTF( "Attempting to close connection at %p\n", (void *)conn->conn );

	if ( conn->type == DB_NONE ) {
		free( conn->conn );
		//fprintf( stderr, "Couldn't close file at: '%s'\n", conn->connstr );
		return 0;	
	}
	else if ( conn->type == DB_SQLITE ) {
		fprintf( stderr, "SQLite3 not done yet.\n" );
		return 0;	
	}
	else if ( conn->type == DB_MYSQL ) {
		mysql_close( (MYSQL *)conn->conn );
	}
	else if ( conn->type == DB_POSTGRESQL ) {
		PQfinish( (PGconn *)conn->conn );
	}

	conn->conn = NULL;
	return 1;
}



//Options
int help () {
	const char *fmt = "%-2s%s --%-24s%-30s\n";
	struct help { const char *sarg, *larg, *desc; } msgs[] = {
		{ "-c", "convert <arg>", "Convert a supplied CSV file <arg> to another format" },
		{ "-e", "headers <arg>", "Only display the headers in <arg>"  },
		{ "-d", "delimiter <arg>", "Specify a delimiter" },
		{ "-r", "root <arg>",  "Specify a $root name for certain types of structures.\n"
			"                              (Example using XML: <$root> <key1></key1> </$root>)" }, 
		{ "",   "name <arg>",  "Synonym for root." },
		{ "-u", "output-delimiter <arg>", "Specify an output delimiter for strings\n"
			"                              (NOTE: VERY useful for SQL)" },
	#if 1
		{ "-t", "typesafe",    "Enforce and/or enable typesafety." },
	#endif
		{ "-f", "format <arg>","Specify a format to convert to" },
		{ "-j", "json",        "Convert into JSON." },
		{ "-x", "xml",         "Convert into XML." },
		{ "-q", "sql",         "Convert into general SQL INSERT statement." },
		{ "-p", "prefix <arg>","Specify a prefix" },
		{ "-s", "suffix <arg>","Specify a suffix" },
		{ "",   "class <arg>", "Generate a class using headers in <arg> as input" },
		{ "",   "struct <arg>","Generate a struct using headers in <arg> as input"  },
		{ "-k", "schema <arg>","Generate an SQL schema using file <arg> as data\n"
      "                              (sqlite is the default backend)" },
		{ "",   "for <arg>",   "Use <arg> as the backend when generating a schema.\n"
			"                              (e.g. for=[ 'oracle', 'postgres', 'mysql',\n" 
		  "                              'sqlserver' or 'sqlite'.])" },
		{ "",   "camel-case",  "Use camel case for struct names" },
		{ "-n", "no-newline",  "Do not generate a newline after each row." },
		{ "",   "no-unsigned", "Remove any unsigned character sequences."  },
		{ "",   "id <arg>",      "Add a unique ID column named <arg>."  },
		{ "",   "add-datestamps","Add columns for date created and date updated"  },
		{ "-a", "ascii",      "Remove any characters that aren't ASCII and reproduce"  },
		{ "-D", "datasource <arg>",   "Use the specified connection string to connect to a database for source data"  },
		{ "-T", "table <arg>",   "Use the specified table when connecting to a database for source data"  },
		{ "-L", "limit <arg>",   "Limit the result count to <arg>"  },
		{ "",   "buffer-size <arg>",  "Process only this many rows at a time when using a database for source data"  },
		{ "-Q", "query <arg>",   "Use this query to filter data from datasource"  },
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
	char *query = NULL;
	dsn_t input, output;
	memset( &input, 0, sizeof( dsn_t ) );

	if ( argc < 2 || !strcmp( *argv, "-h" ) || !strcmp( *argv, "--help" ) ) {
		return help();
	}

	while ( *argv ) {
		if ( !strcmp( *argv, "--no-unsigned" ) )
			no_unsigned = 1;	
		else if ( !strcmp( *argv, "--add-datestamps" ) )
			want_datestamps = 1;	
		else if ( !strcmp( *argv, "-t" ) || !strcmp( *argv, "--typesafe" ) )
			typesafe = 1;
		else if ( !strcmp( *argv, "-n" ) || !strcmp( *argv, "--no-newline" ) )
			newline = 0;
		else if ( !strcmp( *argv, "-j" ) || !strcmp( *argv, "--json" ) )
			stream_fmt = STREAM_JSON;
		else if ( !strcmp( *argv, "-x" ) || !strcmp( *argv, "--xml" ) )
			stream_fmt = STREAM_XML;
		else if ( !strcmp( *argv, "-q" ) || !strcmp( *argv, "--sql" ) )
			stream_fmt = STREAM_SQL;
		else if ( !strcmp( *argv, "" ) || !strcmp( *argv, "--camel-case" ) )
			case_type = CASE_CAMEL;	
		else if ( !strcmp( *argv, "--id" ) ) {
			want_id = 1;	
			if ( !dupval( *( ++argv ), &id_name ) ) {
				return nerr( "%s\n", "No argument specified for --id." );
			}
		}
		else if ( !strcmp( *argv, "-f" ) || !strcmp( *argv, "--format" ) ) {
			if ( !dupval( *( ++argv ), &streamtype ) )
				return nerr( "%s\n", "No argument specified for --format." );
			else if ( ( stream_fmt = check_for_valid_stream( streamtype ) ) == -1 ) {
				return nerr( "Invalid format '%s' requested.\n", streamtype );
			}	
		}
		else if ( !strcmp( *argv, "-k" ) || !strcmp( *argv, "--schema" ) ) {
			schema = 1;
			if ( !dupval( *( ++argv ), &input.connstr ) ) {
				return nerr( "%s\n", "No argument specified for --schema." );
			}
		}
		else if ( !strcmp( *argv, "-r" ) || !strcmp( *argv, "--root" ) ) {
			if ( !dupval( *( ++argv ), &root ) ) {
				return nerr( "%s\n", "No argument specified for --root / --name." );
			}
		}
		else if ( !strcmp( *argv, "--name" ) ) {
			if ( !dupval( *( ++argv ), &root ) ) {
				return nerr( "%s\n", "No argument specified for --name / --root." );
			}
		}
		else if ( !strcmp( *argv, "--for" ) ) {

			if ( *( ++argv ) == NULL ) {
				return nerr( "%s\n", "No argument specified for flag --for." );
			}

			if ( !strcasecmp( *argv, "sqlite3" ) )
				dbengine = SQL_SQLITE3;
			else if ( !strcasecmp( *argv, "postgres" ) )
				dbengine = SQL_POSTGRES;
			else if ( !strcasecmp( *argv, "mysql" ) )
				dbengine = SQL_MYSQL;
			else if ( !strcasecmp( *argv, "mssql" ) )
				dbengine = SQL_MSSQLSRV;
			else if ( !strcasecmp( *argv, "oracle" ) ) {
				dbengine = SQL_ORACLE;
				return nerr( "Oracle SQL dialect is currently unsupported, sorry...\n" );
			}
			else {
				return nerr( "Argument specified '%s' for flag --for is an invalid option.", *argv );
			}

		}
		else if ( !strcmp( *argv, "--class" ) ) {
			classdata = 1;
			case_type = CASE_CAMEL;
			if ( !dupval( *( ++argv ), &input.connstr ) ) {
				return nerr( "%s\n", "No argument specified for --class." );
			}
		}
		else if ( !strcmp( *argv, "--struct" ) ) {
			structdata = 1;
			if ( !dupval( *( ++argv ), &input.connstr ) ) {
				return nerr( "%s\n", "No argument specified for --struct." );
			}
		}
		else if ( !strcmp( *argv, "-a" ) || !strcmp( *argv, "--ascii" ) ) {
			ascii = 1;	
			if ( !dupval( *( ++argv ), &input.connstr ) ) {
				return nerr( "%s\n", "No argument specified for --id." );
			}
		}
		else if ( !strcmp( *argv, "-p" ) || !strcmp( *argv, "--prefix" ) ) {
			if ( !dupval( *(++argv), &prefix ) )
			return nerr( "%s\n", "No argument specified for --prefix." );
		}
		else if ( !strcmp( *argv, "-x" ) || !strcmp( *argv, "--suffix" ) ) {
			if ( !dupval( *(++argv), &suffix ) )
			return nerr( "%s\n", "No argument specified for --suffix." );
		}
		else if ( !strcmp( *argv, "-d" ) || !strcmp( *argv, "--delimiter" ) ) {
			char *del = *(++argv);
			if ( !del )
				return nerr( "%s\n", "No argument specified for --delimiter." );
			else if ( *del == 9 || ( strlen( del ) == 2 && !strcmp( del, "\\t" ) ) )
				DELIM = "\t";
			else if ( !dupval( del, &DELIM ) ) {
				return nerr( "%s\n", "No argument specified for --delimiter." );
			}
		}
		else if ( !strcmp( *argv, "-e" ) || !strcmp( *argv, "--headers" ) ) {
			headers_only = 1;	
			if ( !dupval( *(++argv), &input.connstr ) )
			return nerr( "%s\n", "No file specified with --convert..." );
		}
		else if ( !strcmp( *argv, "-c" ) || !strcmp( *argv, "--convert" ) ) {
			convert = 1;	
			if ( !dupval( *(++argv), &input.connstr ) )
			return nerr( "%s\n", "No file specified with --convert..." );
		}
		else if ( !strcmp( *argv, "-u" ) || !strcmp( *argv, "--output-delimiter" ) ) {
			//Output delimters can be just about anything, 
			//so we don't worry about '-' or '--' in the statement
			if ( ! *( ++argv ) )
				return nerr( "%s\n", "No argument specified for --output-delimiter." );
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
		else if ( !strcmp( *argv, "-D" ) || !strcmp( *argv, "--datasource" ) ) {
			if ( !dupval( *(++argv), &input.connstr ) )
			return nerr( "%s\n", "No argument specified for --datasource." );
		}
		else if ( !strcmp( *argv, "-T" ) || !strcmp( *argv, "--table" ) ) {
			if ( !dupval( *(++argv), &table ) )
			return nerr( "%s\n", "No argument specified for --table." );
		}
		else if ( !strcmp( *argv, "-Q" ) || !strcmp( *argv, "--query" ) ) {
			if ( !dupval( *(++argv), &query ) )
			return nerr( "%s\n", "No argument specified for --query." );
		}
		else if ( !strcmp( *argv, "-o" ) || !strcmp( *argv, "--output" ) ) {
			char *arg = *( ++argv );
			if ( !arg || !dupval( arg, &OUTPUT ) ) {
				return nerr( "%s\n", "No argument specified for --output." );
			}
		}
		argv++;
	}


	// The dsn is always going to be the input
	if ( !parse_dsn_info( &input ) ) {
		fprintf( stderr, "Input file was invalid.\n" );
		return 1;
	}


	// If a command line arg was specified for table, use it
	if ( table ) {
		if ( strlen( table ) < 1 ) {
			fprintf( stderr, "Table name is invalid or unspecified.\n" );
			return 1;
		}

		if ( strlen( table ) >= sizeof( input.tablename ) ) {
			fprintf( stderr, "Table name specified is too long.\n" );
			return 1;
		}

		snprintf( input.tablename, sizeof( input.tablename ), "%s", table ); 
	} 

	// Open the DSN first (regardless of type)	
	if ( !open_dsn( &input, query ) ) {
		fprintf( stderr, "DSN open failed...\n" );
		return 1;
	}

	// There is never a time when the headers aren't needed...
	// Create the schema_t here
	if ( !headers_from_dsn( &input ) ) {
		fprintf( stderr, "Header creation failed...\n" );
		return 1;
	}

	// Processor stuff
	if ( headers_only ) {
		for ( schema_t **s = input.headers; s && *s; s++ ) printf( "%s\n", (*s)->label );
		destroy_dsn_headers( &input );
		close_dsn( &input );
		return 0;
	}

	else if ( schema ) {
		DPRINTF( "Schema only was called...\n" );
		if ( !schema_from_dsn( &input ) ) {
			//if ( !schema_f( FFILE, DELIM ) ) return 1;
			DPRINTF( "Schema build failed...\n" );
			destroy_dsn_headers( &input );
			close_dsn( &input );
			return 1;
		}

		destroy_dsn_headers( &input );
		close_dsn( &input );
		return 0;
	}

	else if ( classdata || structdata ) {
		DPRINTF( "classdata || structdata only was called...\n" );
		//if ( !struct_f( FFILE, DELIM ) ) return 1;
	}

	else if ( convert ) {
		DPRINTF( "convert was called...\n" );
	#if 1
		if ( !prepare_dsn( &input ) ) {
				fprintf( stderr, "Failed to prepare DSN...\n" );
				destroy_dsn_headers( &input );
				close_dsn( &input );
				return 0;
		}
	#endif

		// Control the streaming / buffering from here
		for ( int i = 0; i < 1; i++ ) {

			// Stream to records
			if ( !records_from_dsn( &input, 0, 0 ) ) {
				fprintf( stderr, "Failed to generate records from data source.\n" );
				destroy_dsn_headers( &input );
				close_dsn( &input );
				return 0;
			}

			// Then open a new DSN for the output (there must be an --output flag)
			if ( !OUTPUT ) {
				output.output = stdout; 
			}
			else {
				// Output
				memset( &output, 0, sizeof( dsn_t ) );

				// Parse the output source
				if ( !parse_dsn_info( &output ) ) {
					fprintf( stderr, "Input file was invalid.\n" );
					return 1;
				}
			
			#if 0
				// Test dsn? (to see if the source already exists)
				if ( !test_dsn( &output ) ) {

				}

				// Create an output dsn (unless it already exists)
				if ( !create_dsn( &output ) ) {

				}
			#endif

				// Open the output dsn
				if ( !open_dsn( &output, query ) ) {
					fprintf( stderr, "DSN open failed...\n" );
					return 1;
				}

				// If this is a database, choose the stream type
			#if 0
				if ( output.x == y )
					stream_fmt = STREAM_MYSQL;
				else if ( output.y == x )
					stream_fmt = STREAM_PGSQL;
			#endif
			}

			// What stream is in use?
			// print_stream_type( stream_fmt );

			// Do the transport
			if ( !transform_from_dsn( &input, &output, stream_fmt, 0, 0 ) ) {
				fprintf( stderr, "Failed to transform records from data source.\n" );
				destroy_dsn_headers( &input );
				close_dsn( &input );
				return 0;
			}

			// Block and finish writing before moving further
			fflush( stdout );
exit(0);

		}


		// Check if the user wants types or not
		//if ( !convert_f( FFILE, DELIM, stream_fmt ) ) return 1; //nerr( "conversion of file failed...\n" );
	}
	#if 0
	else if ( transport ) {
		if ( !transport( FFILE, DELIM, "somethin" ) ) {
			return 1;
		}
	}
	#endif
	#if 0
	else if ( ascii ) {
		if ( !ascii_f( FFILE, DELIM, "somethin" ) ) {
			return 1;
		}
	}
	#endif

	
	// Destroy the headers
	destroy_dsn_headers( &input );


	// Close the DSN
	if ( !close_dsn( &input ) ) {
		fprintf( stderr, "DSN close failed...\n" );
		return 1;
	}

	#ifdef DEBUG_H
	print_dsn( &input );
	#endif

#if 0
		// Choose the thing to run and run it
		if ( ds.type == DB_MYSQL ) {
			mysql_exec( &ds, table, query );
		}
		else if ( ds.type == DB_POSTGRESQL ) {
			postgresql_exec( &ds, table, query );
		}
#endif


		// TODO: All of the SQL stuff should return records, and perhaps be broken up more

exit(0);


	//Clean up by freeing everything.
	if ( *DELIM != ',' || strlen( DELIM ) > 1 ) {
		free( DELIM ); 
	}

	if ( strcmp( root, "root" ) != 0 ) {
		free( root );
	} 

	free( FFILE );
	free( prefix );
	free( suffix );
	return 0;
}
