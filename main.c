/* ------------------------------------------------------- *
briggs.c 
========

 * ------------------------------------------------------- */
#define _POSIX_C_SOURCE 200809L
#define VERSION "0.01"
#define TAB "\t\t\t\t\t\t\t\t\t"
#include "vendor/single.h"

//Error message
#define nerr( ... ) \
	(fprintf( stderr, __VA_ARGS__ ) ? 1 : 1 )

#define SHOW_COMPILE_DATE() \
	fprintf( stderr, "briggs v" VERSION " compiled: " __DATE__ ", " __TIME__ "\n" )

#define ADD_ELEMENT( ptr, ptrListSize, eSize, element ) \
	if ( ptr ) \
		ptr = realloc( ptr, sizeof( eSize ) * ( ptrListSize + 1 ) ); \
	else { \
		ptr = malloc( sizeof( eSize ) ); \
	} \
	*(&ptr[ ptrListSize ]) = element; \
	ptrListSize++;

//
typedef enum {

	STREAM_PRINTF = 0
, STREAM_JSON
, STREAM_XML
, STREAM_SQL
, STREAM_CSTRUCT
, STREAM_COMMA
, STREAM_CARRAY
, STREAM_JCLASS
, STREAM_CUSTOM

} Stream;


//Global variables to ease testing
char *prefix = NULL;
char *suffix = NULL;
char *stream_chars = NULL;
char *sqltable = NULL;
int newline=0;
int hlen;
int adv=0;
static const char *ucases = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
static const char *lcases = "abcdefghijklmnopqrstuvwxyz_";
//const char *exchrs = "~`!@#$%^&*()-_+={}[]|:;"'<>,.?/";


//rand number between;
long lbetween (long min, long max) {
	long r = 0;
	do r = rand_number(); while ( r < min || r > max );
	return r;
}

//Convert word
//char *w = strdup( "WORD" );snakecase( &w );
void snakecase ( char **k ) {
	char *nk = *k;
	int p=0;
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

//TODO: Bitshift the first int.  
//control printing comma, control tabbing, control pretty formatting etc
//Simple key-value
void p_default( int ind, const char *k, const char *v ) {
	fprintf( stdout, "%s => %s\n", k, v );
}  

//JSON converter
void p_json( int ind, const char *k, const char *v ) {
	//Check if v is a number, null or some other value that should NOT be escaped
	fprintf( stdout, "%s%s\"%s\": \"%s\"\n", adv ? " " : ",", &TAB[9-ind], k, v );
}  

//C struct converter
void p_carray ( int ind, const char *k, const char *v ) {
	fprintf( stdout, &",\"%s\""[adv], v );
}

//XML converter
void p_xml( int ind, const char *k, const char *v ) {
	fprintf( stdout, "%s<%s>%s</%s>\n", &TAB[9-ind], k, v, k );
}  

//SQL converter
void p_sql( int ind, const char *k, const char *v ) {
	//Check if v is a number, null or some other value that should NOT be escaped
	fprintf( stdout, &",'%s'"[adv], v );
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
	if ( !strlen( vv ) ) {
		fprintf( stdout, &", .%s = \"\"\n"[adv], &TAB[9-ind], k );
		return;
	}
	#if 0
	if ( !strcmp( vv, "true" ) || !strcmp( vv, "TRUE" ) ) {
		fprintf( stdout, "%s.%s = 1,\n", &TAB[9-ind], k );
		return;
	}
		
	if ( !strcmp( vv, "false" ) || !strcmp( vv, "FALSE" ) ) {
		fprintf( stdout, "%s.%s = 0,\n", &TAB[9-ind], k );
		return;
	}
	#endif
	while ( *vv ) {
		if ( !memchr( "0123456789", *vv, 10 ) ) {
			fprintf( stdout, &",%s.%s = \"%s\"\n"[adv], &TAB[9-ind], k, v );
			return;	
		}
		vv++;
	}
	fprintf( stdout, &"%s, .%s = %s\n"[adv], &TAB[9-ind], k, v );
}  


//convert CSV or similar to another format
int convert_f ( const char *file, const char *delim, Stream stream ) {

	int fd;
	char *buf = NULL;
	struct stat b;
	Mem p;
	memset( &p, 0, sizeof(Mem) );

	//stat file
	if ( stat( file, &b ) == -1 ) {
		//should probably be a failure
		fprintf(stderr,"fail...\n"); return 0;	
	}
	
	//open file
	if (( fd = open( file, O_RDONLY )) == -1 ) {
		//fail if file doesn't open
		fprintf(stderr,"fail...\n"); return 0;	
	}

	//alloc
	if (( buf = malloc( b.st_size + 1 )) == NULL ) {
		//fail again
		fprintf(stderr,"fail...\n"); return 0;	
	}

	//read 
	if ( read( fd, buf, b.st_size ) == -1 ) {
		fprintf(stderr,"fail...\n"); return 0;	
	}

	//char add, headers can be in their own, the other should be somewhere else...
	char **headers = NULL;	
	//char **values[];
	int hlen = 0, vlen = 0;

	//Create a new delimiter string...
	char del[ 8 ];
	memset( &del, 0, sizeof(del) );
	memcpy( &del, "\n", 1 );
	memcpy( &del[ 1 ], delim, strlen(delim) );
	//write( 2, del, strlen( del ) ); getchar();
	//write( 2, buf, b.st_size ); getchar();

	//the headers can be the name of the field.
	while ( strwalk( &p, buf, del ) ) {
		if ( p.chr == '\n' ) {
			ADD_ELEMENT( headers, hlen, char *, NULL );
			break;
		}
		else if ( p.chr == ';' ) {
			if ( p.size == 0 ) { 
				//
				ADD_ELEMENT( headers, hlen, char *, strdup( "nothing" ) );
			}
			else {
				char *a = malloc( p.size + 1 );
				memset( a, 0, p.size + 1 );	
				memcpy( a, &buf[p.pos], p.size );
				snakecase( &a );
				ADD_ELEMENT( headers, hlen, char *, a );
			}
		}
	}

#if 0
	while ( *headers ) {
		fprintf( stdout, "%s\n", *headers );
		headers++;
	}
exit(0);
#endif

	//...
	typedef struct { char *k, *v; } Dub;
	Dub *v = NULL, **iv = NULL; 
	void **ov = NULL;
	int odvlen = 0;
	int idvlen = 0;
	int hindex = 0;
	int boink = 0;

	//Theoretically, you don't need to rewind anything...
	while ( strwalk( &p, buf, del ) ) {
		if ( p.chr == '\n' ) {
			if ( iv || boink ) {
				ADD_ELEMENT( iv, idvlen, Dub *, NULL );
				ADD_ELEMENT( ov, odvlen, Dub **, iv );	
				iv = NULL; 
				//headers -= hlen; 
				idvlen = 0;
				hindex = 0;
				boink = 0;
			}
		}
		else if ( p.chr == ';' ) {
			Dub *v = malloc( sizeof( Dub ) );
			v->k = headers[ hindex ];
			//headers++;
			hindex++;
			v->v = malloc( p.size + 1 );
			memset( v->v, 0, p.size + 1 );
			memcpy( v->v, &buf[p.pos], p.size );   
			ADD_ELEMENT( iv, idvlen, Dub *, v );

			if ( hindex > hlen ) {
				boink = 1;	
			}
		}
	}

	//move through the thing
	FILE *output = stdout;
	ADD_ELEMENT( ov, odvlen, Dub **, NULL );	
	int odv = 0;
	while ( *ov ) {
		Dub **bv = (Dub **)*ov;
		//int p = 0;

		//Prefix
		if ( prefix ) {
			fprintf( output, "%s", prefix );
		}

		if ( stream == STREAM_PRINTF )
			0;//p_default( 0, ib->k, ib->v );
		else if ( stream == STREAM_XML )
			0;//p_xml( 0, ib->k, ib->v );
		else if ( stream == STREAM_COMMA )
			0;//fprintf( output, "" );
		else if ( stream == STREAM_CSTRUCT )
			fprintf( output, "%s{", odv ? "," : " "  );
		else if ( stream == STREAM_CARRAY )
			fprintf( output, "%s{ ", odv ? "," : " "  ); 
		else if ( stream == STREAM_SQL )
			fprintf( output, "INSERT INTO %s VALUES ( ", sqltable );
		else if ( stream == STREAM_JSON ) {
			fprintf( output, "%s{", odv ? "," : " "  );
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
				p_xml( 0, ib->k, ib->v );
			else if ( stream == STREAM_CSTRUCT )
				p_cstruct( 1, ib->k, ib->v );
			else if ( stream == STREAM_CARRAY )
				p_carray( 1, ib->k, ib->v );
			else if ( stream == STREAM_COMMA )
				p_comma( 1, ib->k, ib->v );
			else if ( stream == STREAM_SQL )
				p_sql( 0, ib->k, ib->v );
			else if ( stream == STREAM_JSON ) {
				p_json( 0, ib->k, ib->v );
			}
			adv = 0, bv++;
		}

		//Built-in suffix...
		if ( stream == STREAM_PRINTF )
			0;//p_default( 0, ib->k, ib->v );
		else if ( stream == STREAM_XML )
			0;//p_xml( 0, ib->k, ib->v );
		else if ( stream == STREAM_COMMA )
			0;//fprintf( output, " },\n" );
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
			fprintf( output, "\n" );
		}
			
		ov++, odv++;
	}

	return 1;

}


//Options
Option opts[] = {
//	{ "-a", "--directory",  "Simulate receiving this url.",  's' },
//	{ "-b", "--create-db",  "Simulate receiving this url.",      },
//	{ "-d", "--deploy",     "Deploy somewhere [ aws, heroku, linode, shared ]",     's' },
//	{ "-s", "--seed",       "Seed a database." },


	//Stop at this many rows
	//{ "-r", "--rows",       "Define a number of rows.", 'n' },
	//{ "-n", "--newline",    "Override the default delimiter.", 's' },

	//Convert may be all I do from here...
	{ "-c", "--convert",    "Load a file (should be a CSV now)", 's' },
	{ "-d", "--delimiter",  "Specify a delimiter", 's' },
	//{ "-d", "--input-delimiter",  "Specify an input delimiter", 's' },
	//{ "-d", "--output-delimiter",  "Specify an output delimiter", 's' },
	//{ "-e", "--extract",  "Specify fields to extract by name or number", 's' },

	//Serialization formats
	{ "-j", "--json",       "Convert into JSON." },
	{ "-x", "--xml",        "Convert into XML." },
	{ NULL, "--comma",      "Convert into XML." },
	{ NULL, "--carray",     "Convert into a C-style array." },
	{ NULL, "--cstruct",    "Convert into a C struct." },
	{ "-q", "--sql",        "Generate some random XML.", 's' },
	{ "-n", "--newline",    "Generate newline after each row." },
	//{ NULL, "--java-class", "Convert into a Java class." },
	//{ "-m", "--msgpack",    "Generate some random msgpack." },

	//Add prefix and suffix to each row
	{ "-p", "--prefix",     "Specify a prefix", 's' },
	{ "-s", "--suffix",     "Specify a suffix", 's' },
//	{ NULL, "--prefix-value",     "Specify a prefix", 's' },
//	{ NULL, "--suffix-value",     "Specify a suffix", 's' },
	//{ "-b", "--blank",      "Define a default header for blank lines.", 's' },
	//{ NULL, "--no-blank",   "Do not show blank values.", 's' },

	//{ "-h", "--help",      "Show help." },
	{ .sentinel = 1 }
};



int main (int argc, char *argv[]) {
	SHOW_COMPILE_DATE();

	(argc < 2) ? opt_usage(opts, argv[0], "nothing to do.", 0) : opt_eval(opts, argc, argv);

	int stream_fmt = 0;
#if 0
	if ( opt_set( opts, "--rows" ) )
		rows = opt_get(opts, "--rows").n;	

	if ( opt_set( opts, "--seed" ) )
		seeddb(); 

	if ( opt_set( opts, "--json" ) )
		json(); 
#endif

	//Should we be type strict or not?
	if ( opt_set( opts, "--strict" ) )
		0;//stream_fmt = STREAM_JSON;	

	if ( opt_set( opts, "--json" ) )
		stream_fmt = STREAM_JSON;	

	if ( opt_set( opts, "--xml" ) )
		stream_fmt = STREAM_XML;	

	if ( opt_set( opts, "--cstruct" ) )
		stream_fmt = STREAM_CSTRUCT;	

	if ( opt_set( opts, "--comma" ) )
		stream_fmt = STREAM_COMMA;	

	if ( opt_set( opts, "--carray" ) )
		stream_fmt = STREAM_CARRAY;	

	if ( opt_set( opts, "--newline" ) ) {
		//consider being able to take numbers for newlines...
		newline = 1;
	}

	if ( opt_set( opts, "--custom" ) ) {
		stream_fmt = STREAM_CUSTOM;	
		//stream_chars = opt_get( opts, "--custom" ).s;	
	}

	if ( opt_set( opts, "--sql" ) ) {
		stream_fmt = STREAM_SQL;
		sqltable = opt_get( opts, "--sql" ).s;
		if ( !sqltable ) {
			fprintf( stderr, "No SQL table specified for this statement.\n" );
			exit( 1 );
		}
	#if 0
		//TODO: Pretty serious bug in single.c
		//Not detecting '--'
		fprintf( stderr, "%s\n", sqltable );
		if ( *sqltable == '-' ) {
			fprintf( stderr, "Got flag, expected value.\n" );
			exit( 1 );
		}
	#endif
	}

	//
	if ( opt_set( opts, "--prefix" ) ) {
		prefix = opt_get( opts, "--prefix" ).s;	
		//fprintf( stderr, "%s\n", prefix );
	}

	if ( opt_set( opts, "--suffix" ) ) {
		suffix = opt_get( opts, "--suffix" ).s;	
		//fprintf( stderr, "%s\n", suffix );
	}

	if ( opt_set( opts, "--convert" ) ) {
		char *ffile = opt_get( opts, "--convert" ).s;
		char *delim = opt_get( opts, "--delimiter" ).s;
		if ( !delim )
			return nerr( "delimiter not set, stopping...\n" );
		if ( !convert_f( ffile, delim, stream_fmt ) )
			return nerr( "conversion of file failed...\n" );

	}
	return 0;
}
