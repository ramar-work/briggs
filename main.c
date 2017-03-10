/*ezphp.c - Tooling to adminster the framework*/
#define _POSIX_C_SOURCE 200809L
#include "lite.h"
#include "parsely.h"

//Error message
#define err( ... ) \
	(fprintf( stderr, __VA_ARGS__ ) ? 1 : 1 )

//Blog content text
#include "etc.c"

//Global variables to ease testing
char titleBuf[ 1024 ] = { 0 };  /*Titles are no more than 1024 characters*/
int rows              =    10;  /*Assume we want 100 rows in the database*/

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


//Options
Option opts[] = {
//	{ "-a", "--directory",  "Simulate receiving this url.",  's' },
//	{ "-b", "--create-db",  "Simulate receiving this url.",      },
//	{ "-d", "--deploy",     "Deploy somewhere [ aws, heroku, linode, shared ]",     's' },
	{ "-r", "--rows",       "Define a number of rows.", 'n' },
	{ "-s", "--seed",       "Seed a database." },
	{ "-j", "--json",       "Generate some random JSON." },
	{ "-x", "--xml",        "Generate some random XML." },
	{ "-m", "--msgpack",    "Generate some random msgpack." },
	{ .sentinel = 1 }
};



//Some stuff to read from a file...
#if 0
int readme() 
{
	struct stat sb;
	int fd;
	if ( stat( file, &sb ) == -1 )
		return err( "failed to stat file: %s\n", file );
	
	if ((fd = open( file, O_RDONLY )) == -1 )
		return err( "failed to open %s\n", file );

	if (read( fd, buf, sb.st_size ) == -1 )
		return err( "failed to read all of file %s\n", file );

	if ( close ( fd ) == -1 )
		return err( "failed to close file %s\n", file );
}
#endif


//rand number between;
long lbetween (long min, long max)
{
	long r = 0;
	do r = rand_number(); while ( r < min || r > max );
	return r;
}



//generate a randomly long title name
char *title (char *buf, int maxBuff, int words)
{
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

int main (int argc, char *argv[])
{
	(argc < 2) ? opt_usage(opts, argv[0], "nothing to do.", 0) : opt_eval(opts, argc, argv);

	if ( opt_set( opts, "--rows" ) )
		rows = opt_get(opts, "--rows").n;	

	if ( opt_set( opts, "--seed" ) )
		seeddb(); 

	if ( opt_set( opts, "--json" ) )
		json(); 
	return 0;
}
