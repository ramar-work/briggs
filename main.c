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

} Stream;


//Blog content text
//#include "test/etc.c"

//Global variables to ease testing
char titleBuf[ 1024 ] = { 0 };  /*Titles are no more than 1024 characters*/
int rows              =    10;  /*Assume we want 100 rows in the database*/

char *prefix = NULL;
char *suffix = NULL;

//Resource list
static const struct Resources { const char type, *path; } 
resources[] = {
	{ 'd', "models" },
	{ 'd', "views"  },
	{ 'd', "sql"    },
	{ 'd', "static" },
	{ 'd', "vendor" },
	{ 'f', "index.php" },
	{ 'f', "routes.json" },
};


//Load in assets from different places online (pure.css, jquery, etc)
#if 0
static const struct Assets { const char *name, *url; } 
assets[] = {
	{ "pure"  , "https://github.com/yahoo/pure-release/archive/v0.6.2.zip" },
	{ "jquery", "https://code.jquery.com/jquery-3.1.1.min.js" }
};
#endif




//Some stuff to read from a file...
#if 0
int readme() 
{
	struct stat sb;
	int fd;
	if ( stat( file, &sb ) == -1 )
		return nerr( "failed to stat file: %s\n", file );
	
	if ((fd = open( file, O_RDONLY )) == -1 )
		return nerr( "failed to open %s\n", file );

	if (read( fd, buf, sb.st_size ) == -1 )
		return nerr( "failed to read all of file %s\n", file );

	if ( close ( fd ) == -1 )
		return nerr( "failed to close file %s\n", file );
}
#endif


//rand number between;
long lbetween (long min, long max) {
	long r = 0;
	do r = rand_number(); while ( r < min || r > max );
	return r;
}



//generate a randomly long title name
char *title (char *buf, int maxBuff, int words) {
	int p = 0, 
			w = !words ? 1 : words;

	for ( int ii=0; (p + 1) > maxBuff || ii<w; ii++ )
		p += sprintf( &buf[ p ], "%s ", titles[rand_number() % (sizeof(titles) / sizeof(char *))]);

	buf[ p - 1 ] = '\0';
	return buf;
}



//Quick random generate
int seeddb (void) 
{
	struct timespec tspec;
	clock_gettime( CLOCK_REALTIME, &tspec ); 
	int authorMax  = sizeof( authors ) / sizeof(char *);
	int titleMax   = sizeof( titles ) / sizeof(char *);
	int contentMax = sizeof( content ) / sizeof(char *) ;
	FILE *out      = stdout;

	const int       dMin = 915166800;                 /*Jan 01, 1999*/
	const int       dMax = tspec.tv_sec;              /*Jan 01, 1999*/
	const char *iAuthors = "INSERT INTO authors VALUES "
                         "(NULL, '%s', '%s', '%s', %d, %d);\n";
	const char *iPosts   = "INSERT INTO posts VALUES "
		                     "( NULL, '%s', %d, %d, %d );\n"; 
	const char *iContent = "INSERT INTO content VALUES "
                         "(NULL, %d, %d, '%s', %d, %d);\n";

	Parser p = { .words = {{"\n"},{NULL}}};
	pr_prepare( &p );

	//Write the load statement 
	fprintf( out, "%s", loadSql );

	//Add all these authors just because
	for (int i=0 ; i < authorMax; i++ ) 
		fprintf( out, iAuthors, authors[i], authorIDs[i], pwds[i],
			lbetween( dMin, dMax ), lbetween( dMin, dMax ));

	//Seed the post database
	for (int i=0; i < rows; i++ )
	{
		//Decide on a post name, author and what not
		int aLimit  = rand_number() % authorMax; //(int)lbetween( 1, authorMax );
		int cLimit  = rand_number() % contentMax; //(int)lbetween( 1, contentMax );
		long dAdded = lbetween( dMin, dMax - 3600 ); 
		long dMod   = lbetween( dAdded, dMax - 3600 ); 
		char *t     = title( titleBuf, sizeof(titleBuf), rand_number() % 6 );

		//Output the post add statement
		fprintf( out, iPosts, t, aLimit, dAdded, dMod );
		
		//Then generate the content inserts.
		for ( int ii=cLimit, cid=0; ii < sizeof(content)/sizeof(char *); ii++, cid++ ) 
		{
			long dAdded = lbetween( dMin, dMax - 3600 ); 
			long dMod   = lbetween( dAdded, dMax - 3600 ); 
			fprintf( out, iContent, i, cid, content[ii], dAdded, dMod ); 
		}
	}

	return 1;	
}


int json (void)
{
	struct timespec tspec;
	clock_gettime( CLOCK_REALTIME, &tspec ); 
	int authorMax  = sizeof( authors ) / sizeof(char *);
	int titleMax   = sizeof( titles ) / sizeof(char *);
	int contentMax = sizeof( content ) / sizeof(char *) ;
	FILE *out      = stdout;

	const int       dMin = 915166800;                 /*Jan 01, 1999*/
	const int       dMax = tspec.tv_sec;              /*Jan 01, 1999*/
	const char *iAuthors = "\"author\": { \"%s\", \"%s\", \"%s\", %d, %d },\n";
	const char *iPosts   = "\t\"post\": { \"title\": \"%s\", \"index\": \"%s\", \"count\": %d, \"dateAdded\": %d, \"dateModified\": %d },\n";
	const char *iContent = "\t\"content\": { %d, \"%d\", \"%s\", %d, %d },\n";
	const char *iRandArr = "\t\"claus\": [ \"%s\", %d, %d, %d ],\n";
                         
	Parser p = { .words = {{"\n"},{NULL}}};
	pr_prepare( &p );

#if 0
	//Add all these authors just because
	for (int i=0 ; i < authorMax; i++ ) 
		fprintf( out, iAuthors, authors[i], authorIDs[i], pwds[i],
			lbetween( dMin, dMax ), lbetween( dMin, dMax ));
#endif
	//Seed the post database
	for (int i=0; i < rows; i++ )
	{
		fprintf( out, "{\n" );
		//Decide on a post name, author and what not
		int aLimit  = rand_number() % authorMax; //(int)lbetween( 1, authorMax );
		int cLimit  = rand_number() % contentMax; //(int)lbetween( 1, contentMax );
		long dAdded = lbetween( dMin, dMax - 3600 ); 
		long dMod   = lbetween( dAdded, dMax - 3600 ); 
		char *t     = title( titleBuf, sizeof(titleBuf), rand_number() % 6 );

		
		//Then generate the content inserts.
		for ( int ii=cLimit, cid=0; ii < sizeof(content)/sizeof(char *); ii++, cid++ ) 
		{
			long dAdded = lbetween( dMin, dMax - 3600 ); 
			long dMod   = lbetween( dAdded, dMax - 3600 ); 
		fprintf( out, iPosts, t, rand_chars(12), aLimit, dAdded, dMod );
			fprintf( out, iContent, i, cid, content[ii], dAdded, dMod ); 
			fprintf( out, iRandArr, rand_chars(10), rand_number(), rand_number(), rand_number() );
		}
		fprintf( out, "},\n" );
	}

	return 1;
}


int xml (void)
{
	struct timespec tspec;
	clock_gettime( CLOCK_REALTIME, &tspec ); 
	int authorMax  = sizeof( authors ) / sizeof(char *);
	int titleMax   = sizeof( titles ) / sizeof(char *);
	int contentMax = sizeof( content ) / sizeof(char *) ;
	FILE *out      = stdout;

	const int       dMin = 915166800;                 /*Jan 01, 1999*/
	const int       dMax = tspec.tv_sec;              /*Jan 01, 1999*/
	const char *iAuthors = "\"author\": { \"%s\", \"%s\", \"%s\", %d, %d },\n";
	const char *iPosts   = "\t\"post\": { \"title\": \"index\": \"%s\", \"count\": %d, \"dateAdded\": %d, \"dateModified\": %d },\n";
	const char *iContent = "\t\"content\": { %d, \"%d\", \"%s\", %d, %d },\n";
	const char *iRandArr = "\t\"claus\": [ \"%s\", %d, %d, %d ],\n";
                         
	Parser p = { .words = {{"\n"},{NULL}}};
	pr_prepare( &p );

#if 0
	//Add all these authors just because
	for (int i=0 ; i < authorMax; i++ ) 
		fprintf( out, iAuthors, authors[i], authorIDs[i], pwds[i],
			lbetween( dMin, dMax ), lbetween( dMin, dMax ));
#endif
	//Seed the post database
	for (int i=0; i < rows; i++ )
	{
		fprintf( out, "{\n" );
		//Decide on a post name, author and what not
		int aLimit  = rand_number() % authorMax; //(int)lbetween( 1, authorMax );
		int cLimit  = rand_number() % contentMax; //(int)lbetween( 1, contentMax );
		long dAdded = lbetween( dMin, dMax - 3600 ); 
		long dMod   = lbetween( dAdded, dMax - 3600 ); 
		char *t     = title( titleBuf, sizeof(titleBuf), rand_number() % 6 );

		//Output the post add statement
		fprintf( out, iPosts, t, aLimit, dAdded, dMod );
		
		//Then generate the content inserts.
		for ( int ii=cLimit, cid=0; ii < sizeof(content)/sizeof(char *); ii++, cid++ ) 
		{
			long dAdded = lbetween( dMin, dMax - 3600 ); 
			long dMod   = lbetween( dAdded, dMax - 3600 ); 
			fprintf( out, iContent, i, cid, content[ii], dAdded, dMod ); 
			fprintf( out, iRandArr, rand_chars(10), rand_number(), rand_number(), rand_number() );
		}
		fprintf( out, "},\n" );
	}

	return 1;
}


struct Ghost 
{
	/*Something to print out before the loop begins*/
	const char *pre;
	/*Start and end encapsulators for each loop*/
	const char *start, *end;
	const char *blocks[10];
};

const char *blocks[] = {
	"\"author\": { \"%s\", \"%s\", \"%s\", %d, %d },\n",
	"\t\"post\": { \"title\": \"%s\", \"index\": \"%s\", \"count\": %d, \"dateAdded\": %d, \"dateModified\": %d },\n",
	"\t\"content\": { %d, \"%d\", \"%s\", %d, %d },\n",
	"\t\"claus\": [ \"%s\", %d, %d, %d ],\n"
};


//Simple key-value
void p_default( int ind, const char *k, const char *v ) {
	fprintf( stdout, "%s => %s\n", k, v );
}  


//JSON converter
void p_json( int ind, const char *k, const char *v ) {
	//Check if v is a number, null or some other value that should NOT be escaped
	fprintf( stdout, "%s\"%s\": \"%s\",\n", &TAB[9-ind], k, v );
}  

//XML converter
void p_xml( int ind, const char *k, const char *v ) {
	fprintf( stdout, "%s<%s>%s</%s>\n", &TAB[9-ind], k, v, k );
}  

//SQL converter
void p_sql( int ind, const char *k, const char *v ) {
	//Check if v is a number, null or some other value that should NOT be escaped
	fprintf( stdout, "'%s', ", v );
}  

//C struct converter
void p_cstruct ( int ind, const char *k, const char *v ) {

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
				ADD_ELEMENT( headers, hlen, char *, strdup( "nothing" ) );
			}
			else {
				char *a = malloc( p.size + 1 );
				memset( a, 0, p.size + 1 );	
				memcpy( a, &buf[p.pos], p.size );
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

#if 1
	//move through the thing
	ADD_ELEMENT( ov, odvlen, Dub **, NULL );	
	while ( *ov ) {
		Dub **bv = (Dub **)*ov;
		int p = 0;

		//Prefix
		if ( prefix ) {
			fprintf( stderr, "%s\n", prefix );
		}

		if ( stream == STREAM_PRINTF )
			0;//p_default( 0, ib->k, ib->v );
		else if ( stream == STREAM_XML )
			0;//p_xml( 0, ib->k, ib->v );
		else if ( stream == STREAM_JSON ) {
			fprintf( stdout, "{\n" );
		}

		//The body
		while ( *bv && (*bv)->k ) {
			Dub *ib = *bv;
			if ( stream == STREAM_PRINTF )
				p_default( 0, ib->k, ib->v );
			else if ( stream == STREAM_XML )
				p_xml( 0, ib->k, ib->v );
			else if ( stream == STREAM_JSON ) {
				p_json( 0, ib->k, ib->v );
			}
			bv++;
		}

		//Suffix
		if ( stream == STREAM_PRINTF )
			0;//p_default( 0, ib->k, ib->v );
		else if ( stream == STREAM_XML )
			0;//p_xml( 0, ib->k, ib->v );
		else if ( stream == STREAM_JSON ) {
			//wipe the last ','
			fprintf( stdout, "},\n" );
			//fprintf( stdout, "," );
		}

		//Suffix
		if ( suffix ) {
			fprintf( stderr, "%s\n", suffix );
		}

		ov++;
	}
#endif

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
	//{ "-e", "--extract",  "Specify fields to extract by name or number", 's' },

	//Serialization formats
	{ "-j", "--json",       "Convert into JSON." },
	{ "-x", "--xml",        "Convert into XML." },
	//{ "-q", "--sql",        "Generate some random XML." },
	//{ "-m", "--msgpack",    "Generate some random msgpack." },

	//Add prefix and suffix to each row
	{ "-p", "--prefix",     "Specify a prefix", 's' },
	{ "-s", "--suffix",     "Specify a suffix", 's' },
	//{ "-b", "--blank",      "Define a default header for blank lines.", 's' },
	//{ NULL, "--no-blank",   "Do not show blank values.", 's' },

	//{ "-h", "--help",      "Show help." },
	{ .sentinel = 1 }
};



int main (int argc, char *argv[])
{
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

	if ( opt_set( opts, "--json" ) )
		stream_fmt = STREAM_JSON;	

	if ( opt_set( opts, "--xml" ) )
		stream_fmt = STREAM_XML;	

	if ( opt_set( opts, "--prefix" ) ) {
		prefix = opt_get( opts, "--prefix" ).s;	
		fprintf( stderr, "%s\n", prefix );
	}

	if ( opt_set( opts, "--suffix" ) ) {
		suffix = opt_get( opts, "--suffix" ).s;	
		fprintf( stderr, "%s\n", suffix );
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
