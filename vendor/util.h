#ifndef _WIN32
 #define _POSIX_C_SOURCE 200809L
#endif 

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "../vendor/zwalker.h"

#ifndef UTIL_H
#define UTIL_H

#ifdef DEBUG_H
 #define FPRINTF(...) \
	fprintf( stderr, "DEBUG: %s[%d]: ", __FILE__, __LINE__ ) && \
	fprintf( stderr, __VA_ARGS__ )

 #if 0
 #define add_item(LIST,ELEMENT,SIZE,LEN) \
	fprintf( stderr, "%s[%d]: ", __FILE__, __LINE__ ) && \
	fprintf( stderr, "Adding a new item %p to list: %p\n", (void *)ELEMENT, (void *)LIST ) && \
		add_item_to_list( (void ***)LIST, ELEMENT, sizeof( SIZE ), LEN )
 #endif
 #define add_item(LIST,ELEMENT,SIZE,LEN) \
	add_item_to_list( (void ***)LIST, ELEMENT, sizeof( SIZE ), LEN )
#else
 #define FPRINTF(...)
 #define add_item(LIST,ELEMENT,SIZE,LEN) \
	add_item_to_list( (void ***)LIST, ELEMENT, sizeof( SIZE ), LEN )
#endif

#define ENCLOSE(SRC, POS, LEN) \
	write( 2, "'", 1 ); \
	write( 2, &SRC[ POS ], LEN ); \
	write( 2, "'\n", 2 );

#define CREATEITEM(TPTR,SIZE,SPTR,HASH,LEN) \
	struct rb *TPTR = malloc( sizeof( SIZE ) ); \
	memset( TPTR, 0, sizeof( SIZE ) ); \
	TPTR->len = LEN;	\
	TPTR->ptr = SPTR; \
	TPTR->hash = HASH; \
	TPTR->rbptr = NULL;

#define srand_nums(BUF,BUFLEN) \
	(char *)srand_uint8t( (unsigned char *)"0123456789", sizeof("0123456789"), (unsigned char *)BUF, BUFLEN )

#define srand_letters(BUF,BUFLEN) \
	(char *)srand_uint8t( (unsigned char *)"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", \
		sizeof("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"), (unsigned char *)BUF, BUFLEN )

#define srand_chars(BUF,BUFLEN) \
	(char *)srand_uint8t( (unsigned char *)"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", \
		sizeof("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"), (unsigned char *)BUF, BUFLEN )

#define mrand_nums(BUFLEN) \
	(char *)srand_uint8t( (unsigned char *)"0123456789", sizeof("0123456789"), malloc(BUFLEN), BUFLEN )

#define mrand_chars(BUFLEN) \
	(char *)srand_uint8t( (unsigned char *)"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", \
		sizeof("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"), malloc(BUFLEN), BUFLEN )

#define mrand_letters(BUFLEN) \
	(char *)srand_uint8t( (unsigned char *)"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", \
		sizeof("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"), malloc(BUFLEN), BUFLEN )

#define append_to_char(DEST,DESTLEN,SRC) \
	append_to_uint8t( (unsigned char **)DEST,DESTLEN,(unsigned char *)SRC,strlen(SRC) )

unsigned char *read_file ( const char *filename, int *len, char *err, int errlen );
int safeatoi( const char *value );
char *get_lstr( char **str, char chr, int *lt );
char *msg_get_value ( const char *value, const char *chrs, unsigned char *msg, int len );
char *copystr ( unsigned char *src, int len ) ;
unsigned char *append_to_uint8t ( unsigned char **, int *, unsigned char *, int ); 
unsigned char * srand_uint8t( unsigned char *, int, unsigned char *, int );
void *add_item_to_list( void ***, void *, int , int * );
char *append_strings_to_char (char **dest, int *len, char *delim, ... );
unsigned char *trim (unsigned char *msg, char *trim, int len, int *nlen);
#endif
