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


typedef struct kv { 
  char *k; 
  char *v; 
} Dub;


typedef struct Functor {
	const char *type;
  const char *left_fmt;
  const char *right_fmt;
  int i;
  void (*execute)( int, const char *, const char * ); 
  void *p;
} Functor;


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
int hlen = 0;
int no_unsigned=0;
int adv=0;
const char ucases[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
const char lcases[] = "abcdefghijklmnopqrstuvwxyz_";
const char exchrs[] = "~`!@#$%^&*()-_+={}[]|:;\"'<>,.?/";


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


//Simple key-value
void p_default( int ind, const char *k, const char *v ) {
	fprintf( stdout, "%s => %s%s%s\n", k, ld, v, rd );
}  


//JSON converter
void p_json( int ind, const char *k, const char *v ) {
	//Check if v is a number, null or some other value that should NOT be escaped
	fprintf( stdout, "%s%c\"%s\": \"%s\"\n", &TAB[9-ind], adv ? ' ' : ',', k, v );
}  


//C struct converter
void p_carray ( int ind, const char *k, const char *v ) {
	fprintf( stdout, &", \"%s\""[ adv ], v );
}


//XML converter
void p_xml( int ind, const char *k, const char *v ) {
	fprintf( stdout, "%s<%s>%s</%s>\n", &TAB[9-ind], k, v, k );
}  


//SQL converter
void p_sql( int ind, const char *k, const char *v ) {
	//Check if v is a number, null or some other value that should NOT be escaped
	fprintf( stdout, &",%s%s%s"[adv], ld, v, rd );
}  


//Simple comma converter
void p_comma ( int ind, const char *k, const char *v ) {
	//Check if v is a number, null or some other value that should NOT be escaped
	fprintf( stdout, &",\"%s\""[adv], v );
}


//C struct converter
void p_cstruct ( int ind, const char *k, const char *v ) {
	//check that it's a number
	char *vv = (char *)v;

	//Handle blank values
	if ( !strlen( vv ) ) {
		fprintf( stdout, "%s", &TAB[9-ind] );
		fprintf( stdout, &", .%s = \"\"\n"[ adv ], k );
		return;
	}

#if 1
	fprintf( stdout, "%s", &TAB[9-ind] );
	fprintf( stdout, &", .%s = \"%s\"\n"[adv], k, v );
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


//Copy a string off as snake_case
char * copy_and_snakecase( char *src, int size ) {
	char *b = NULL; 
	char *a = malloc( size + 1 );
	int nsize = 0;
	memset( a, 0, size + 1 );
	b = (char *)trim( (unsigned char *)src, " ", size, &nsize );
	memcpy( a, b, nsize );
	snakecase( &a );
	return a;
} 


//Extract the headers from a CSV
char ** generate_headers( char *buf, char *del ) {
	char **headers = NULL;	
	zWalker p = { 0 };
	while ( strwalk( &p, buf, del ) ) {
		char *t = ( !p.size ) ? no_header : copy_and_snakecase( &buf[ p.pos ], p.size - 1 );
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


void free_inner_records( Dub **v ) {
	Dub **iv = v;	
	while ( *iv ) {
		free( (*iv)->v );
		free( *iv );
		iv++;
	}
	free( v );
}


void free_records ( Dub ***v ) {
	Dub ***ov = v;
	while ( *ov ) {
		free_inner_records( *ov );
		ov++;
	}
	free( v );
}


Dub *** generate_records ( char *buf, char *del, char **headers ) {
	Dub **iv = NULL, ***ov = NULL;
	int ol = 0, il = 0, hindex = 0;
	zWalker p = { 0 };

	//Find the first '\n' and skip it
	while ( *(buf++) != '\n' );

	//Now allocate for values
	while ( strwalk( &p, buf, del ) ) {
		//fprintf( stderr, "%d, %d\n", p.chr, *p.ptr );
		if ( p.chr == '\r' ) 
			continue;
		else if ( hindex >= hlen ) {
			( !ov ) ? free_inner_records( iv ) : free_records( ov );
			return NULL;
		}

		Dub *v = malloc( sizeof( Dub ) );
		v->k = headers[ hindex ];
		v->v = malloc( p.size + 1 );
		memset( v->v, 0, p.size + 1 );
		extract_value_from_column( &buf[ p.pos ], &v->v, p.size - 1 );	
		add_item( &iv, v, Dub *, &il );
		hindex++;

		if ( p.chr == '\n' ) {
			add_item( &ov, iv, Dub **, &ol );	
			iv = NULL, il = 0, hindex = 0;
		}
	}

	//This ensures that the final row is added to list...
	//TODO: It'd be nice to have this within the above loop.
	add_item( &ov, iv, Dub **, &ol );	
	return ov;
}



void output_records ( Dub ***ov, FILE *output, Stream stream ) {
	int odv = 0;
	while ( *ov ) {
		Dub **bv = *ov;

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
			Dub **hv = *ov;
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
			Dub *ib = *bv;
			//Sadly, you'll have to use a buffer...
			//Or, use hlen, since that should contain a count of columns...
			if ( stream == STREAM_PRINTF )
				p_default( 0, ib->k, ib->v );
			else if ( stream == STREAM_XML )
				p_xml( 1, ib->k, ib->v );
			else if ( stream == STREAM_CSTRUCT )
				p_cstruct( 1, ib->k, ib->v );
			else if ( stream == STREAM_CARRAY )
				p_carray( 0, ib->k, ib->v );
			else if ( stream == STREAM_COMMA )
				p_comma( 1, ib->k, ib->v );
			else if ( stream == STREAM_SQL )
				p_sql( 0, ib->k, ib->v );
			else if ( stream == STREAM_JSON ) {
				p_json( 1, ib->k, ib->v );
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


int headers_f ( const char *file, const char *delim ) {
	char **h = NULL, **headers = NULL;
	char *buf = NULL; 
	int buflen = 0;
	char err[ 2048 ] = { 0 };
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


//convert CSV or similar to another format
int convert_f ( const char *file, const char *delim, Stream stream ) {
	char *buf = NULL, **headers = NULL;
 	char err[2048] = { 0 }, del[8] = { '\r', '\n', 0, 0, 0, 0, 0, 0 };
	Dub ***ov = NULL;
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

	if ( !( ov = generate_records( buf, del, headers ) ) ) {
		free_headers( headers );
		free( buf );
		return nerr( "%s\n", "There seem to be more columns than headers in this file, stopping...\n" );
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
		{ "-u", "output-delimiter <arg>", "Specify an output delimiter for strings\n"
			"                              (NOTE: VERY useful for SQL)" },
		{ "-f", "format <arg>","Specify a format to convert to" },
		{ "-j", "json",        "Convert into JSON." },
		{ "-x", "xml",         "Convert into XML." },
		{ "-q", "sql",         "Convert into general SQL INSERT statement." },
		{ "-p", "prefix <arg>","Specify a prefix" },
		{ "-s", "suffix <arg>","Specify a suffix" },
		{ "-n", "no-newline",  "Do not generate a newline after each row." },
		{ "",   "no-unsigned", "Remove any unsigned character sequences."  },
		{ "-h", "help",        "Show help." },

	#if 0
		{ "-t", "typesafe",    "Attempt to be typesafe with conversion." },
	#endif
	};

	for ( int i = 0; i < sizeof(msgs) / sizeof(struct help); i++ ) {
		struct help h = msgs[i];
		fprintf( stderr, fmt, h.sarg, strlen( h.sarg ) ? "," : " ", h.larg, h.desc );
	}
	return 0;
}


int main (int argc, char *argv[]) {
	SHOW_COMPILE_DATE();
	int stream_fmt = STREAM_PRINTF;

	if ( argc < 2 || !strcmp( *argv, "-h" ) || !strcmp( *argv, "--help" ) ) {
		return help();
	}

	while ( *argv ) {
		if ( !strcmp( *argv, "--no-unsigned" ) )
			no_unsigned = 1;	
		#if 0
		else if ( !strcmp( *argv, "-t" ) || !strcmp( *argv, "--typesafe" ) )
			typesafe = 1;	
		#endif
		else if ( !strcmp( *argv, "-n" ) || !strcmp( *argv, "--no-newline" ) )
			newline = 0;
		#if 0
		#endif
		else if ( !strcmp( *argv, "-j" ) || !strcmp( *argv, "--json" ) )
			stream_fmt = STREAM_JSON;
		else if ( !strcmp( *argv, "-x" ) || !strcmp( *argv, "--xml" ) )
			stream_fmt = STREAM_XML;
		else if ( !strcmp( *argv, "-q" ) || !strcmp( *argv, "--sql" ) )
			stream_fmt = STREAM_SQL;
		else if ( !strcmp( *argv, "-f" ) || !strcmp( *argv, "--format" ) ) {
			if ( !dupval( *( ++argv ), &streamtype ) )
				return nerr( "%s\n", "No argument specified for --format." );
			else if ( ( stream_fmt = check_for_valid_stream( streamtype ) ) == -1 ) {
				return nerr( "Invalid format '%s' requested.\n", streamtype );
			}	
		}
		else if ( !strcmp( *argv, "-r" ) || !strcmp( *argv, "--root" ) ) {
			if ( !dupval( *( ++argv ), &root ) ) {
				return nerr( "%s\n", "No argument specified for --root." );
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
	else if ( convert ) {
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
