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

#define nerr( ... ) \
	(fprintf( stderr, "briggs: " ) ? 1 : 1 ) && (fprintf( stderr, __VA_ARGS__ ) ? 0 : 0 )

#define SHOW_COMPILE_DATE() \
	fprintf( stderr, "briggs v" VERSION " compiled: " __DATE__ ", " __TIME__ "\n" )

#ifdef WIN32
 #define NEWLINE "\r\n"
#else
 #define NEWLINE "\n"
#endif

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
} Stream;



typedef enum case_t {
	NONE = 0,
	CASE_SNAKE,
	CASE_CAMEL
} case_t;


typedef enum type_t {
	T_NULL = 0,
	T_CHAR,
	T_STRING,
	T_INTEGER,
	T_DOUBLE,
	T_BOOLEAN,
	/* T_BINARY // If integrating this, consider using a converter */
} type_t;


typedef struct kv { 
  char *k; 
  char *v; 

	type_t type;
	type_t exptype;

} record_t;


typedef struct Functor {
	const char *type;
  const char *left_fmt;
  const char *right_fmt;
  int i;
  void (*execute)( int, record_t * ); 
  void *p;
} Functor;


typedef struct sqldef_t {
	const char *id_fmt;
	const char *coldefs[8];
} sqldef_t;


typedef enum sqltype_t {
	SQLITE3,
	POSTGRES,
	ORACLE,
	MSSQLSRV,
	MYSQL	
} sqltype_t;


sqldef_t coldefs[1] = {
	{ "%s INTEGER PRIMARY KEY AUTOINCREMENT", { "NULL",	"TEXT",	"TEXT",	"INTEGER","REAL","INTEGER", NULL, } },
};


//Global variables to ease testing
char *no_header = "nothing";
char *output_file = NULL;
char *FFILE = NULL;
char *DELIM = ",";
char *prefix = NULL;
char *suffix = NULL;
char *stream_chars = NULL;
char *root = "root";
char *streamtype = NULL;
char *ld = "'", *rd = "'", *od = "'";
struct rep { char o, r; } ; //Ghetto replacement scheme...
struct rep **reps = NULL;
int headers_only = 0;
int convert=0;
int newline=1;
int typesafe=0;
int schema=0;
int hlen = 0;
int no_unsigned=0;
int adv=0;
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


//Convert word to snake_case
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


//Convert word to camelCase
static void camelcase ( char **k ) {
	char *nk = *k;
	char *ok = *k;
	int first = 0;
	int cap = 0;
	int len = strlen( *k );
	int plen = 0;

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


//Duplicate a value
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


//Simple key-value
void p_default( int ind, record_t *r ) {
	fprintf( stdout, "%s => %s%s%s\n", r->k, ld, r->v, rd );
}  


//JSON converter
void p_json( int ind, record_t *r ) {
	//Check if v is a number, null or some other value that should NOT be escaped
	fprintf( stdout, 
		"%s%c\"%s\": \"%s\"\n", &TAB[9-ind], adv ? ' ' : ',', r->k, r->v );
}  


//C struct converter
void p_carray ( int ind, record_t *r ) {
	fprintf( stdout, &", \"%s\""[ adv ], r->v );
}


//XML converter
void p_xml( int ind, record_t *r ) {
	fprintf( stdout, "%s<%s>%s</%s>\n", &TAB[9-ind], r->k, r->v, r->k );
}


//SQL converter
void p_sql( int ind, record_t *r ) {
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


//Simple comma converter
void p_comma ( int ind, record_t *r ) {
	//Check if v is a number, null or some other value that should NOT be escaped
	fprintf( stdout, &",\"%s\""[adv], r->v );
}


//C struct converter
void p_cstruct ( int ind, record_t *r ) {
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


//Trim and copy a string, optionally using snake_case
char * copy( char *src, int size, case_t t ) {
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
fprintf( stderr, "KEY: '" );
fprintf( stderr, "%s", a );
//write( 2, a, nsize );
fprintf( stderr, "'\n" );
	}
	return a;
}



//Extract the headers from a CSV
char ** generate_headers( char *buf, char *del ) {
	char **headers = NULL;	
	zWalker p = { 0 };
	while ( strwalk( &p, buf, del ) ) {
		char *t = ( !p.size ) ? no_header : copy( &buf[ p.pos ], p.size - 1, case_type );
		add_item( &headers, t, char *, &hlen );
		if ( p.chr == '\n' || p.chr == '\r' ) break;
	}
	return headers;
}



//Free headers allocated previously
void free_headers ( char **headers ) {
	char **h = headers;
	while ( *h ) {
		free( *h );
		h++;
	}
	free( headers );
}



//Get a value from a column
void extract_value_from_column ( char *src, char **dest, int size ) {
	if ( !reps && !no_unsigned ) 
		memcpy( *dest, src, size );
	else {
		char *pp = src;
		int k=0;
		//just do two seperate passes... 
		//TODO: wow, this is bad...
		if ( no_unsigned ) {
			for ( int i=0; i < size; i++ ) {
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

			for ( int i=0, l=0; i<k; i++ ) {
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


void free_inner_records( record_t **v ) {
	record_t **iv = v;	
	while ( *iv ) {
		free( (*iv)->v );
		free( *iv );
		iv++;
	}
	free( v );
}



void free_records ( record_t ***v ) {
	record_t ***ov = v;
	while ( *ov ) {
		free_inner_records( *ov );
		ov++;
	}
	free( v );
}



record_t *** generate_records ( char *buf, char *del, char **headers, char *err ) {
	record_t **iv = NULL, ***ov = NULL;
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
			( !ov ) ? free_inner_records( iv ) : free_records( ov );
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
		extract_value_from_column( &buf[ p.pos ], &v->v, p.size - 1 );	

		// Do the typecheck ehre
		if ( typesafe ) {
			v->exptype = *(gtypes[ hindex ]);
			v->type = get_type( v->v, v->exptype );

			if ( typesafe && v->exptype != v->type ) {
				( !ov ) ? free_inner_records( iv ) : free_records( ov );
				snprintf( err, ERRLEN - 1, 
					"Type check for value at column '%s', line %d failed: %s != %s ( '%s' )\n", 
					v->k, line, sqlite_coldefs[(int)v->exptype], sqlite_coldefs[(int)v->type], 
					v->v );
				return NULL;
			}
		}

		add_item( &iv, v, record_t *, &il );
		hindex++;

		if ( p.chr == '\n' ) {
			add_item( &ov, iv, record_t **, &ol );	
			iv = NULL, il = 0, hindex = 0, line++;
		}
	}

	//This ensures that the final row is added to list...
	//TODO: It'd be nice to have this within the above loop.
	add_item( &ov, iv, record_t **, &ol );	
	return ov;
}



void output_records ( record_t ***ov, FILE *output, Stream stream ) {
	int odv = 0;
	while ( *ov ) {
		record_t **bv = *ov;

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


Functor f[] = {
	[STREAM_PRINTF ] = { "printf", NULL, NULL, 0, p_default, NULL },
	[STREAM_XML    ] = { "xml", NULL, NULL, 0, p_xml, NULL },
	[STREAM_COMMA  ] = { "comma", NULL, NULL, 1, p_comma, NULL },
	[STREAM_CSTRUCT] = { "c-struct", NULL, "}", 1, p_cstruct, NULL },
	[STREAM_CARRAY ] = { "c-array", NULL, " }", 1, p_carray, NULL },
	[STREAM_SQL    ] = { "sql", NULL, " );", 0, p_sql, NULL },
	[STREAM_JSON   ] = { "json", NULL, "}", 0, p_json, NULL },
#if 0
	[STREAM_JAVA] = { "json" NULL, "}", 0, p_json, NULL },
#endif
};



//check for a valid stream
int check_for_valid_stream ( char *st ) {
	for ( int i = 0; i < sizeof(f) / sizeof(Functor); i++ ) {
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
	char *str = malloc( slen );
	memset( str, 0, slen );
	memcpy( str, start, slen );
	return str;
}



// Return all the types
static type_t ** get_types ( char *str, const char *delim ) {
	//... 
	type_t **ts = NULL;
	int tlen = 0;
	zWalker p = { 0 };
//fprintf( stderr, "STR: '%s', DEL: '%s'\n", str, delim );

	while ( strwalk( &p, str, delim ) ) {
//fprintf( stderr, "ONE: '%s'\n", str );
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
			//fprintf( stderr, "%s -> %s ( %d )\n", *j, "", (int)*p); j++;
			continue;
		}

		type_t type = get_type( t, T_STRING ); 
		type_t *x = malloc( sizeof( type_t ) );
		memset( x, 0, sizeof( type_t ) );
		*x = type;

		//fprintf( stderr, "%s -> %s ( %d )\n", *j, t, (int)type ); j++;
		add_item( &ts, x, type_t *, &tlen );
	}

	return ts;
}

int headers_f ( const char *file, const char *delim ) {
	char **h = NULL, **headers = NULL;
	char *buf = NULL; 
	int buflen = 0;
	char err[ ERRLEN ] = { 0 };
	char del[ 8 ] = { '\r', '\n', 0, 0, 0, 0, 0, 0 };

	//Create the new delimiter string.
	memcpy( &del[ strlen( del ) ], delim, strlen(delim) );

	if ( !( buf = (char *)read_file( file, &buflen, err, sizeof( err ) ) ) ) {
		return nerr( "%s", err );
	}

	if ( !( h = headers = generate_headers( buf, del ) ) ) {
		free( buf );
		return nerr( "Failed to generate headers." );
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
	int buflen = 0;
	char err[ ERRLEN ] = { 0 };
	char del[ 8 ] = { '\r', '\n', 0, 0, 0, 0, 0, 0 };
	
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

//write( 2, start, slen ); exit(0);

	//
	char *str = extract_row( buf, buflen );
	if ( !str ) {
		free( buf );
		return nerr( "Failed to extract first row..." );
	}

	type_t **types = get_types( str, del );

	// Root should have been specified
	// If not, I don't know...
	fprintf( stdout, "CREATE TABLE %s (\n", root );

	// Add an ID if we specify it? (unsure how to do this)
	if ( want_id ) {
		fprintf( stdout, "%s INTEGER PRIMARY KEY AUTOINCREMENT\n", id_name );
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
		char *type = sqlite_coldefs[ (int)**types ];
		//fprintf( stderr, "%s -> %s\n", *j, type );
		( adv == 1 ) ? 0 : fprintf( stdout, "," );
		fprintf( stdout, "%s %s\n", *h, type );
		free( *h ), free( *types );
		h++, types++, adv = 0;
	}

	fprintf( stdout, ");\n" );

	//TODO: Free the header list
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
exit(0);
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
	free( headers );
	free( buf );
	return 1;
}



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
#if 0
	while ( strwalk( &p, buf, del ) ) {
		char *t = ( !p.size ) ? no_header : copy_and_snakecase( &buf[ p.pos ], p.size - 1 );
		add_item( &headers, t, char *, &hlen );
		if ( p.chr == '\n' || p.chr == '\r' ) break;
	}
#endif
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



//convert CSV or similar to another format
int convert_f ( const char *file, const char *delim, Stream stream ) {
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



//Options
int help () {
	const char *fmt = "%-2s%s --%-24s%-30s\n";
	struct help { const char *sarg, *larg, *desc; } msgs[] = {
		{ "-c", "convert <arg>", "Convert a supplied CSV file <arg> to another format" },
		{ "-e", "headers <arg>", "Only display the headers in <arg>"  },
		{ "-d", "delimiter <arg>", "Specify a delimiter" },
		{ "-r", "root <arg>",  "Specify a $root name for certain types of structures.\n"
			"                              (Example using XML: <$root> <key1></key1> </$root>)" }, 
		{ "",   "name <arg>",  "Synonym for root.\n" },
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
		{ "",   "class <arg>", "Generate a class using headers in <arg> as input\n" },
		{ "",   "struct <arg>","Generate a struct using headers in <arg> as input\n"  },
		{ "-k", "schema <arg>","Generate an SQL schema using file <arg> as data\n"
      "                              (sqlite is the default backend)" },
		{ "",   "for <arg>",   "Use <arg> as the backend when generating a schema.\n"
			"                              (e.g. for=[ 'oracle', 'postgres', 'mysql',\n" 
		  "                              'sqlserver' or 'sqlite'.])" },
		{ "",   "camel-case",  "Use camel case for struct names\n" },
		{ "-n", "no-newline",  "Do not generate a newline after each row." },
		{ "",   "no-unsigned", "Remove any unsigned character sequences."  },
		{ "",   "id <arg>",      "Add a unique ID column named <arg>."  },
		{ "",   "add-datestamps","Add columns for date created and date updated"  },
		{ "-h", "help",        "Show help." },
	};

	for ( int i = 0; i < sizeof(msgs) / sizeof(struct help); i++ ) {
		struct help h = msgs[i];
		fprintf( stderr, fmt, h.sarg, strlen( h.sarg ) ? "," : " ", h.larg, h.desc );
	}
	return 0;
}



int main (int argc, char *argv[]) {
	SHOW_COMPILE_DATE();

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
			if ( !dupval( *( ++argv ), &FFILE ) ) {
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
		else if ( !strcmp( *argv, "--class" ) ) {
			classdata = 1;
			case_type = CASE_CAMEL;
			if ( !dupval( *( ++argv ), &FFILE ) ) {
				return nerr( "%s\n", "No argument specified for --class." );
			}
		}
		else if ( !strcmp( *argv, "--struct" ) ) {
			structdata = 1;
			if ( !dupval( *( ++argv ), &FFILE ) ) {
				return nerr( "%s\n", "No argument specified for --struct." );
			}
		}
		else if ( !strcmp( *argv, "-p" ) || !strcmp( *argv, "--prefix" ) ) {
			if ( !dupval( *(++argv), &prefix ) ) {
				return nerr( "%s\n", "No argument specified for --prefix." );
			}
		}
		else if ( !strcmp( *argv, "-s" ) || !strcmp( *argv, "--suffix" ) ) {
			if ( !dupval( *(++argv), &suffix ) ) {
				return nerr( "%s\n", "No argument specified for --suffix." );
			}
		}
		else if ( !strcmp( *argv, "-d" ) || !strcmp( *argv, "--delimiter" ) ) {
			if ( !dupval( *(++argv), &DELIM ) ) {
				return nerr( "%s\n", "No argument specified for --delimiter." );
			}
		}
		else if ( !strcmp( *argv, "-e" ) || !strcmp( *argv, "--headers" ) ) {
			headers_only = 1;	
			if ( !dupval( *(++argv), &FFILE ) ) {
				return nerr( "%s\n", "No file specified with --convert..." );
			}
		}
		else if ( !strcmp( *argv, "-c" ) || !strcmp( *argv, "--convert" ) ) {
			convert = 1;	
			if ( !dupval( *(++argv), &FFILE ) ) {
				return nerr( "%s\n", "No file specified with --convert..." );
			}
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
		argv++;
	}

	if ( headers_only ) {
		if ( !headers_f( FFILE, DELIM ) ) {
			return 1; //nerr( "conversion of file failed...\n" );
		}
	}
	else if ( schema ) {
		if ( !schema_f( FFILE, DELIM ) ) {
			return 1;
		}
	}
	else if ( classdata || structdata ) {
		if ( !struct_f( FFILE, DELIM ) ) {
			return 1;
		}
	}
	else if ( convert ) {
		// Check if the user wants types or not
		if ( !convert_f( FFILE, DELIM, stream_fmt ) ) {
			return 1; //nerr( "conversion of file failed...\n" );
		}
	}

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
