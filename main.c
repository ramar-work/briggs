/* ------------------------------------------------------- *
briggs.c 
========

TODO
- Handle unsigned characters
- Handle Unicode (uint32_t?)
- Generate your own values:
	one flag ought to be used to generate some things
	if I give it a file, use a single line
	if I give it multiple values, use those
	if I give it a string, then do yet something else...
	briggs -t range=[ x, y, z ] 
- Test data (especially for C/W.net)
	- Numbers
	- Zips
	- Addresses
	- Names
	- Words
	- Paragraphs
	- URLs
	- Images (probably from a directory is the most useful)
	
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
struct rep { char o, r; } ; //Ghetto replacement scheme...
struct rep **reps = NULL;
char repblock[64]={0};
int replen = 0;
int headers_only = 0;
int newline=0;
int hlen;
int no_unsigned=0;
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



//Data that comes from our little data folder.
extern const char *WORDS[];
extern const char *ADDRESSES[];

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
		fprintf(stderr,"fail...\n"); 
		return 0;	
	}
	
	//open file
	if (( fd = open( file, O_RDONLY )) == -1 ) {
		//fail if file doesn't open
		fprintf(stderr,"fail...\n"); 
		return 0;	
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


	//write(2,buf,b.st_size);

	//char add, headers can be in their own, the other should be somewhere else...
	char **headers = NULL;	
	//char **values[];
	int hlen = 0, vlen = 0;

	//Create a new delimiter string...
	if ( strlen( delim ) > 7 ) {
		return nerr( "delimiter too big (exceeds max of 7 chars), stopping...\n" );
		exit( 1 );
	}
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
	#ifdef WIN32
		#error "briggs is currently unsupported on Windows."
		else if  (p.chr == '\r' ) {
			0;
		}
	#endif
		else { //if ( p.chr == ';' ) {
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

#if 1
	if ( headers_only ) {
		while ( *headers ) {
			fprintf( stdout, "%s\n", *headers );
			headers++;
		}
		//exit( 0 );
	}
#endif

	//...
	typedef struct { char *k, *v; } Dub;
	Dub *v = NULL, **iv = NULL; 
	void **ov = NULL;
	int odvlen = 0;
	int idvlen = 0;
	int hindex = 0;
	int boink = 0;

	//Now allocate for values
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
	#ifdef WIN32
		#error "briggs is currently unsupported on Windows."
		else if  (p.chr == '\r' ) {
			0;
		}
	#endif
		else { /*if ( p.chr == ';' ) {*/
			Dub *v = malloc( sizeof( Dub ) );
			v->k = headers[ hindex ];
			//headers++;
			hindex++;
			v->v = malloc( p.size + 1 );
			memset( v->v, 0, p.size + 1 );

			//Check reps
	#if 0
			memcpy( v->v, &buf[p.pos], p.size );   
	#else
			if ( !reps && !no_unsigned ) 
				memcpy( v->v, &buf[p.pos], p.size );   
			else {
				char *pp = &buf[ p.pos ];
				int k=0;
				//just do two seperate passes... 
				//TODO: wow, this is bad...
				if ( no_unsigned ) {
					for ( int i=0; i<p.size; i++ ) {
						if ( pp[ i ] < 32 || pp[ i ] > 126 )
							0;// v->v[ i ] = ' ';
						else {
							v->v[ k ] = pp[ i ];
							k++;
						}
					}
				}
		
				if ( reps ) {
					if ( !k ) {
						k = p.size;
					}
					else {
						pp = v->v;
					}

					for ( int i=0, l=0; i<k; i++ ) {
						int j = 0;
						struct rep **r = reps;
						while ( (*r)->o ) {
							if ( pp[ i ] == (*r)->o ) {
								j = 1;		
								if ( (*r)->r != 0 ) {
									v->v[ l ] = (*r)->r;
									l++;
								}
								break;
							}
							r++;
						}
						if ( !j ) {
							v->v[ l ] = pp[ i ];
							l++;
						}
					}
					//zero out n<p.size
				}
			}
#endif
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



//Get a random element from a big list...
int get_random_element ( char **list, char **item, int size ) {
	//el[ $RANDOM % size ];
	//copy to item...
	return 0;
}


int get_random_dir ( DIR *dir ) {

	return 0;
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
	{ NULL, "--cut",  "Cut parts of a string", 's' },
	{ NULL, "--headers-only","Only display the headers" },
	//{ "-d", "--input-delimiter",  "Specify an input delimiter", 's' },
	//{ "-d", "--output-delimiter",  "Specify an output delimiter", 's' },
	//{ "-e", "--extract",  "Specify fields to extract by name or number", 's' },

	//Serialization formats
	{ "-j", "--json",       "Convert into JSON." },
	{ "-x", "--xml",        "Convert into XML." },
	{ NULL, "--comma",      "Convert into XML." },
	{ NULL, "--carray",     "Convert into a C-style array." },
	{ NULL, "--cstruct",    "Convert into a C struct." },
	{ NULL, "--cfml",       "Convert into CFML structs." },
	{ NULL, "--no-unsigned","Remove any unsigned character sequences." },
	{ "-q", "--sql",        "Generate some random XML.", 's' },
	{ "-n", "--newline",    "Generate newline after each row." },
	//{ NULL, "--java-class", "Convert into a Java class." },
	//{ "-m", "--msgpack",    "Generate some random msgpack." },

	//Add prefix and suffix to each row
	{ "-p", "--prefix",     "Specify a prefix", 's' },
	{ "-s", "--suffix",     "Specify a suffix", 's' },
	{ "-f", "--format",     "Specify a format for randomization", 's' },
	{ "-d", "--dir",     "Specify a suffix", 's' },
//	{ NULL, "--prefix-value",     "Specify a prefix", 's' },
//	{ NULL, "--suffix-value",     "Specify a suffix", 's' },
	//{ "-b", "--blank",      "Define a default header for blank lines.", 's' },
	//{ NULL, "--no-blank",   "Do not show blank values.", 's' },

	{ "-h", "--help",      "Show help." },
	{ .sentinel = 1 }
};



int main (int argc, char *argv[]) {
	SHOW_COMPILE_DATE();

	(argc < 2) ? opt_usage(opts, argv[0], "nothing to do.", 0) : opt_eval(opts, argc, argv);
	if ( opt_set( opts, "--help" ) ) {
		opt_usage(opts, argv[0], "Help:", 0);	
	}

	int stream_fmt = 0;
#if 0
	if ( opt_set( opts, "--rows" ) )
		rows = opt_get(opts, "--rows").n;	

	if ( opt_set( opts, "--seed" ) )
		seeddb(); 

	if ( opt_set( opts, "--json" ) )
		json(); 
#endif


	if ( opt_set( opts, "--no-unsigned" ) )
		no_unsigned = 1;	

	if ( opt_set( opts, "--headers-only" ) )
		headers_only = 1;	

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

	//Detect characters to cut here...
	if ( opt_set( opts, "--cut" ) ) {
		char *sopts;
		Mem cc;

		//TODO: This is obviously not a good call... no uint8_t allowed
		if ( !(sopts = opt_get( opts, "--cut" ).s) ) {
			fprintf(stderr,"--cut got no arguments...\n" );
			return 1;
		}

		//Allocate and move through much garbage.
		while ( strwalk( &cc, sopts, "," ) ) {
			//If no '=', just get rid of it	
			int p = memchrat( &sopts[ cc.pos ], '=', cc.size );
			struct rep *r = malloc( sizeof( struct rep ) );
			memset( r, 0, sizeof( struct rep ) );
			//If so, Break a string according to =
			if ( p == -1 )
				r->o = sopts[cc.pos], r->r = 0;	
			//Save ONE char only on both sides
			else {
				r->o = sopts[cc.pos];
				r->r = sopts[cc.pos + (p+1)];
			}	
			ADD_ELEMENT( reps, replen, struct rep *, r );
		}

		if ( replen ) {
			struct rep *r = malloc( sizeof( struct rep ) );
			r->r = 0;
			ADD_ELEMENT( reps, replen, struct rep *, r );
		}

	#if 0
		for ( int i=0; i<4; i++ ) {
			struct rep **r = reps;
			while ( (*r)->r ) {
				fprintf( stderr, "%c -> '%c'\n", (*r)->o, (*r)->r );
				//fprintf( stderr, "%c -> '%c'\n", r->o, r->r );
				r++;
			}
		}

		for ( int i=0; i<replen; i++ ) {
			fprintf( stderr, "%c -> '%c'\n", reps[ i ]->o, reps[i]->r );
		}
	#endif
	}

	if ( opt_set( opts, "--convert" ) ) {
		char *ffile = opt_get( opts, "--convert" ).s;
		char *delim = opt_get( opts, "--delimiter" ).s;
	#ifdef DEBUG
		fprintf(stderr,"Got delim %s\n",delim);
	#endif

		if ( !delim )
			return nerr( "delimiter not set, stopping...\n" );

	#ifdef DEBUG
		fprintf(stderr,"Caling convert_f\n" );
	#endif

		if ( !convert_f( ffile, delim, stream_fmt ) ) {
			return nerr( "conversion of file failed...\n" );
		}
	}
	return 0;
}
