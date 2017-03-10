/* ------------------------------------------------------ *
lite.h
-----

Summary
-------
Lite is a library that should just drop in to most C projects 
and in the future, C++ (though it's not nearly as useful and 
you might as well just rely on the stdlib).

To undefine things and make the 
library small, just DEFINE the
modules you don't want.  This is a
weird semantic thing, I know, but
I figured it was easier than typing
NO_LITE_X everywhere, and everything
stays in the same namespace.


Basic utilities ship with this:
- Buffers?
- Linked lists
- Error mapping
- Sockets
- Memory walkers (and eventually strings)
- Options

Stuff I want to add:
* Opaque key-value store
	* worked on this a number of times
* Memory tracking (ew)
* Hash tables and whatnot
* Formatter
* Queues
* Stacks
* Renderer or tokenizer
  * done in another module (still heavy)
* Super simple framebuffer renderer
  * ?
* Filesystem tools
	* they're prototyped, but not done
* Basic crypto
	* md5 / crc / sha1sum / base64 (super common stuff)
* drop in database (even lighter than SQLite)



 * ------------------------------------------------------ */

/*Headers*/
#define _POSIX_C_SOURCE 200809L
#ifndef LITE_H
#define LITE_H
#define LITE_BUFFER
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

//Defining a module will NOT compile it.

//Set experimental flags here
#ifndef LITE_EXPERIMENTAL
 #define LITE_STATE
 #define LITE_MT
 #define LITE_EXPERIMENTAL
#else
 #undef LITE_EXPERIMENTAL
#endif

#ifndef LITE_MEM
 #define LITE_MEM
 #include <string.h>
 #include <strings.h>

 //Initialize a memory tracker structure
 #define meminit(mems, p, m) \
	Mem mems; \
	memset(&mems, 0, sizeof(Mem)); \
	mems.pos = p; \
	mems.it = m; 

#else
 #undef LITE_MEM
#endif

#ifndef LITE_TIMER
 #define LITE_TIMER
 #include <time.h>

	/*TODO: Consider using VA_ARGS here to make this less clumsy.  
    The second argument will be resolution, 
		the third will be the name of the thing
	  Even better yet if type could be deduced (a letter first 
    means pop char * into label, a number first means otherwise, 
		anything else is type error*/
 #define tprint(t) __timer_eprint(t)
 #define tlabel(t, n) __timer_set_name(t, n)
 #ifdef LITE_VERBOSE_TIMER
  #define tstart(t, y) __timer_init(t, y); __timer_start(t, __FILE__, __LINE__)
  #define tfinish(t) __timer_end(t, __FILE__, __LINE__)
  #define telap(t) __timer_elap(t)
 #else
  #define tstart(t, y) __timer_init(t,y); __timer_start(t)
  #define tfinish(t) __timer_end(t)
  #define telap(t) __timer_elap(t)
 #endif
#else
 #undef LITE_TIMER
 #define tstart(t, y)
 #define tfinish(t)
 #define telap(t)
 #define tprint(t)
 #define tlabel(t, n)
#endif

#ifndef LITE_FILES
 #define LITE_FILES
 #include <sys/stat.h>
#else
 #undef LITE_FILES
#endif


#ifndef LITE_BUFFER
 #define LITE_BUFFER
#else
 #undef LITE_BUFFER
#endif

#ifndef LITE_LIST
 #define LITE_LIST
#else
 #undef LITE_LIST
#endif

#ifndef LITE_RANDOM
 #define LITE_RANDOM
 #include <time.h>
#else
 #undef LITE_RANDOM
#endif

#ifndef LITE_TOKENIZER
 #define LITE_TOKENIZER
#else
 #undef LITE_TOKENIZER
#endif

#ifndef LITE_SOCKET
 #define LITE_SOCKET
 #include <netinet/in.h>
 #include <fcntl.h>
 #include <sys/socket.h>
 #include <sys/un.h>
 #include <arpa/inet.h>
 #include <netdb.h>
#else
 #undef LITE_SOCKET
#endif

#ifndef LITE_OPT
 #define LITE_OPT
#else
 #undef LITE_OPT
#endif

#ifndef LITE_HASH
 #define LITE_HASH
 #define LITEU32_CMP_LEN 12
 #define LITEU32_MASK 0x7ffffff
#else
 #undef LITE_HASH
#endif

#ifndef LITE_MT
 #define LITE_MT
#else
 #undef LITE_MT
#endif

#ifndef LITE_FMT
 #define LITE_FMT
 //Print string using name of variable as key
 #define nsprintf(v) \
	fprintf(stderr, "%-30s: '%s'\n", #v, v)

 //Print number using name of variable as key
 #define niprintf(v) \
	fprintf(stderr, "%-30s:  %d\n", #v, v)

 //Print long using name of variable as key
 #define nlprintf(v) \
	fprintf(stderr, "%-30s: %ld\n", #v, v)

 //Print float using name of variable as key
 #define nfprintf(v) \
	fprintf(stderr, "%-30s: %f\n", #v, v)

 //Print pointer using name of variable as key
 #define npprintf(v) \
	fprintf(stderr, "%-30s: %p\n", #v, (void *)v)

 //Print binary data (in hex) using name of variable as key
 #define nbprintf(v, n) \
	fprintf (stderr,"%-30s: ", k); \
	for (int i=0; i < n; i++) fprintf( stderr, "%02x", v[i] ); \
	fprintf (stderr, "\n")

 //Print strings
 #define stprintf(k, v) \
	fprintf(stderr, "%-30s: '%s'\n", k, v)

 //Print numbers
 #define nmprintf(k, v) \
	fprintf(stderr, "%-30s:  %d\n", k, v)

 //Print large numbers 
 #define ldprintf(k, v) \
	fprintf(stderr, "%-30s: %ld\n", k, v)

 //Print float / double 
 #define fdprintf(k, v) \
	fprintf(stderr, "%-30s: %f\n", k, v)

 //Print pointers 
 #define spprintf(k, v) \
	fprintf(stderr, "%-30s: %p\n", k, (void *)v)

 //Print binary data (in hex)
 #define bdprintf(k, v, n) \
	fprintf (stderr,"%-30s: ", k); \
	for (int i=0; i < n; i++) fprintf( stderr, "%02x", v[i] ); \
	fprintf (stderr, "\n")

 //Finally, print the size of things
 #define szprintf(k) \
	fprintf(stderr, "Size of %-22s: %ld\n", #k, sizeof(k));
#else
 #undef LITE_FMT
#endif

#ifndef LITE_STATE
 #define LITE_STATE
 //Track memory allocations
 #define nalloc(x, y) malloc(x)
#else
 #undef LITE_STATE
#endif


//Datatypes
typedef struct ErrorMap { int nargs; const char *msg; } ErrorMap;

typedef struct Error Error;
struct Error { char *str, *name; FILE *file; };


#ifdef LITE_TIMER
//Different time types
typedef enum { 
	LITE_NSEC = 0,
	LITE_USEC,
	LITE_MSEC,
	LITE_SEC,
} LiteTimetype;

typedef struct Timer Timer;
struct Timer {	
	clockid_t   clockid;     
	int         linestart,   
              lineend;	  
	struct 
  timespec    start,     
              end;      
	const char *label; 
 #ifdef CV_VERBOSE_TIMER 
  const char *file;        
 #endif
	LiteTimetype  type;     
};

#endif


#ifdef LITE_BUFFER
typedef struct Buffer Buffer;
struct Buffer {
	uint8_t *body;
	uint32_t pos;
	uint32_t size;
};
#endif


#ifdef LITE_LIST
typedef struct ListNode ListNode;
struct ListNode {
	void *item;
	ListNode *next;
};

typedef struct List List;
struct List {
	uint32_t count;
	uint32_t element_size;
	void     (*free)(void *self);  /*Remember how this works.*/
	ListNode *head;
	ListNode *curr;
	ListNode *tail;
}; 
#endif


#ifdef LITE_RANDOM
 #define RAND_BUF_SIZE 64 
typedef struct Random Random;
struct Random {
	struct timespec rand_ts;
	char   buf[RAND_BUF_SIZE];
};
#endif


#ifdef LITE_MEM
/*Custom memory datatype*/
typedef struct Mem Mem;
struct Mem {
	int   pos,   /*Position*/
        next,  /*Next position*/
        size;  /*Size of something*/
	int   it;
	uint8_t chr;   /*Character found*/
};
#endif


#ifdef LITE_OPT
typedef struct Value Value;
struct Value {
	#if 0
	union { int32_t n; char *s; char c; void *v; } value;
	#endif
	int32_t n; char *s; char c; void *v; 
};

typedef struct Option Option;
struct Option {
	const char  *sht, *lng; 
	const char  *description;
	char  type; /*n, s or c*/	
	_Bool set;  /*If set is not bool, it can define the order of demos*/
	Value v;
	_Bool (*callback)(char **av, Value *v, char *err);
	_Bool sentinel;
};
#endif


#ifdef LITE_FILES 
/*Define this within the C file as a static structure.  No one else knows this exists*/
typedef struct OSInfo OSInfo;
struct OSInfo {
	uint8_t  dirsep;  /* '/' or '\' */
	uint32_t win_filename_limit;
	uint32_t nix_filename_limit;
	uint32_t msg_lim;
	uint32_t max_dir_depth;
};

typedef struct FileInfo FileInfo;
struct FileInfo {
	uint32_t mode;
	uint32_t inode;
	uint32_t device;
	uint32_t uid;
	uint32_t gid;
	uint32_t atime;
	uint32_t ctime;
	uint32_t mtime;
	uint32_t links;
	uint32_t size;
	uint32_t bs;
	uint32_t bsalloc;
	char type;
	char * extension;
	char * path;
	char * basename;
	char * dirname;
	char * realpath;
	char * filetype;
};
#endif


#ifdef LITE_STATE
typedef struct Reference Reference;
struct Reference {
	const char *name;
	unsigned int size;	
	void *ptr;
};

typedef struct Tracker Tracker;
struct Tracker {
	char *file;
	char *fname;      /*Specify the current function name */
	char *msg;        /*A buffer for error messages */
 #ifndef LITE_LIST
	List *elements;   /*Keep track of allocated elements*/
 #endif
	uint32_t line;
	uint32_t memlim;  /*Specify a bound on memory usage */
	uint32_t rc;      /*The return code of most recently executed function */
	uint32_t last;    /*The last allocated element */
};
#endif


#ifdef LITE_SOCKET
typedef struct Socket Socket;
struct Socket {
	/*User settable stuff*/
	_Bool server;        //Is this a server or client?
	char *proto;         //Is this udp or tcp?
	int port;   //Define a port
	char *hostname;      //Hostname to BIND to

	/*...*/
	int domain;              /* AF_INET, AF_INET6, AF_UNIX */
	int protocol;            /* Transport protocol to use. */
	int conntype;            /* TCP, UDP, UNIX, etc. (sock_stream) */
	int err;

	/*Non user settable stuff*/
	unsigned int connections;/* How many people have tried to connect to you? */
	
	/* Socket settings */
	unsigned int bufsz;             /* Size of read buffer */
	unsigned int backlog;           /* Number of queued connections. */
	unsigned int waittime;          /* There is a way to query socket, but 
                                      do this for now to ensure your sanity. */
	char opened;             /* True or false? */

	/* More data */
	char *service;                  /* A string representation of service */
	char _class;                    /* Listener, receiver or child? */
	struct addrinfo hints;          /* Hints on addresses */
	struct addrinfo *res;           /* Results of address call. */
#if 0
	Buffer *buffer;                 /* A buffer */
	//OBJECT(urn) *urn;               /* A URI parser */
#endif

	/* Server information -  used by bind() */
	int fd;                      /* Parent file descriptor */
	char portstr[6];                /* Write it here */
	struct sockaddr *srvaddr;       /* Server address structure */ 
	struct sockaddr_in tmpaddrinfo;
	struct sockaddr_in *srvaddrinfo;  /* Server's address information */
	socklen_t srvaddrlen;           /* Size of the above structure */
	char address[INET6_ADDRSTRLEN]; /* Address gets stored here */
	size_t addrsize;                /* Address information size */

	/* Client information - used by accept() */
	int clifd;                      /* Child file descriptor */
	struct sockaddr *cliaddr;       /* Client address structure */
	struct sockaddr_in *cliaddrinfo;  /* Client's address information */
	socklen_t cliaddrlen;           /* Size of the above structure */

	/* SSL / TLS */
	void *ssl_ctx;                  /* If SSL is in use, use this */
} socket_t;
#endif


#ifdef LITE_HASH
//Define a Hash Table
typedef struct HashTable HashTable;
struct HashTable { 
	void  **data;
	char   *check;
	_Bool (*cmp)(const void *, const void *); 
	int     slots;
} ;
#endif


/*Error handling and logging.*/
void errname (char *msg); 
void errfile (FILE *file);


/*Definitions for verbose & non-verbose printing*/
#ifdef VVPRINTF 
 int errstatusf (char *file, int line, int code);
 void * erritemf (char *file, int line, void *p);
 void * __errnull (char *file, int line, char *msg); 
 void __errstep (char *file, int line, char *msg); 
 int __errstat (char *file, int line, int code, char *msg);
 void __errexit (char *file, int line, int code, char *msg);
 int __errfree (char *file, int line, int code, void *p, char *msg);
 int __errsys (char *file, int line, char *msg);

 #define errnull(x) __errnull(__FILE__, __LINE__, x)
 #define errstep(x) __errstep(__FILE__, __LINE__, x)
 #define errstat(x, y) __errstat(__FILE__, __LINE__, x, y)
 #define errexit(x, y) __errexit(__FILE__, __LINE__, x, y)
 #define errfree(x, y, z) __errfree(__FILE__, __LINE__, x, y, z)
 #define errsys(x) __errsys(__FILE__, __LINE__, x)
 
 #define STAT_STR "%s [line %d]: "
 #define SUCCESS \
 	errstatusf(__FILE__, __LINE__, 1)
 #define INITIALIZED(p) \
 	erritemf(__FILE__, __LINE__, p)
 /* the idea here is to increment automatically when you reach a point */
 /* you can probably use a global array to track this... it's so ugly */
 #define iiprintf(p) \
 	vvprintf("Initializing %s...", #p)
 #define FAILURE \
 	errstatusf(__FILE__, __LINE__, 0)
 #define vvprintf(...) \
 	fprintf(stderr, STAT_STR, __FILE__, __LINE__); \
 	fprintf(stderr, __VA_ARGS__); \
 	fprintf(stderr, "\n")
 #define eeprintf(...) \
 	fprintf(stderr, "%s\n", __VA_ARGS__)

#else
 void * __errnull (char *msg); 
 void __errstep (char *msg); 
 int __errstat (int code, char *msg);
 void __errexit (int code, char *msg);
 int __errfree (int code, void *p, char *msg);
 int __errsys (char *msg);
 
 #define errnull(x) __errnull(x)
 #define errstep(x) __errstep(x)
 #define errstat(x, y) __errstat(x, y)
 #define errexit(x, y) __errexit(x, y)
 #define errfree(x, y, z) __errfree(x, y, z)
 #define errsys(x) __errsys(x)

 #define STAT_STR
 #define SUCCESS 1
 #define iiprintf(p) p
 #define INITIALIZING(p) p
 #define FAILURE 0
 #define vvprintf(...)
 #define eeprintf(...)
#endif


#ifdef LITE_BUFFER
Buffer * buffer_init (uint32_t size);
_Bool buffer_append (Buffer *self, void *body, uint32_t size);
_Bool buffer_prepend(Buffer *self, void *body, uint32_t size);
_Bool buffer_insert (Buffer *self, void *body, uint32_t size);
//uint32_t buffer_write (Buffer *self, char *fn, uint32_t s, uint32_t e);
//uint32_t buffer_read (Buffer *self, void *p, uint32_t tsize, uint32_t bsize);
#endif


#ifdef LITE_SOCKET 
 /*Socket*/
 //void socket_info(struct SOCKET *self);
 void socket_free(Socket *self);
_Bool socket_open (Socket *sock);
 void socket_printf (Socket *sock);
_Bool socket_accept (Socket *sock, Socket *new);
_Bool socket_tcp_recv (Socket *sock, uint8_t *msg, uint32_t *len);
_Bool socket_tcp_rrecv (Socket *sock, uint8_t *msg, int get, uint32_t *len);
_Bool socket_tcp_send (Socket *sock, uint8_t *msg, uint32_t len);
//_Bool socket_udp_recv (Socket *sock, uint8_t *msg, uint32_t *len);
//_Bool socket_udp_send (Socket *sock, uint8_t *msg, uint32_t len);

unsigned int socket_connect(Socket *self, const char *hostname, unsigned int port);
 //buffer_t * socket_recvd(Socket *self);
void socket_addrinfo(Socket *self);
_Bool socket_bind(Socket *self);
_Bool socket_listen(Socket *self);
//uri_t * socket_parsed(Socket *self);
_Bool socket_shutdown(Socket *self, char *type);
_Bool socket_close(Socket *self);
_Bool socket_recv(Socket *self); 
 //Socket * socket_accept(struct SOCKET *self);
_Bool socket_send(Socket *self, char *msg, unsigned int length); 
void socket_release(Socket *self);
#endif


#ifdef LITE_MEM 
_Bool memstr (const void * a, const void *b, int size);
int32_t memchrocc (const void *a, const char b, int32_t size);
int32_t memstrocc (const void *a, const void *b, int32_t size);
int32_t memstrat (const void *a, const void *b, int32_t size);
int32_t memchrat (const void *a, const char b, int32_t size);
int32_t memtok (const void *a, const uint8_t *tokens, int32_t rng, int32_t tlen);
int32_t memmatch (const void *a, const char *tokens, int32_t sz, char delim); 
char *memstrcpy (char *dest, const uint8_t *src, int32_t len);
_Bool memwalk (Mem *mm, uint8_t *data, uint8_t *tokens, int datalen, int toklen) ;
#endif


#ifdef LITE_OPT 
_Bool opt_usage (Option *, const char *, const char *, int);
_Bool opt_eval (Option *opts, int argc, char **argv);
 //union opt_value *get(Option *opts, const char *name);	
_Bool opt_set (Option *opts, char *flag); 
Value opt_get (Option *opts, char *flag); 
#endif


#ifdef LITE_FILES 
 //
char file_type    (FileInfo *self, const char *path);
char * file_basename(FileInfo *self, const char *path);
char * file_dirname (FileInfo *self, const char *path);
char * file_realpath(FileInfo *self, const char *path);
 // LIST * list    (FileInfo *self, const char *path, file_t *file[]); 
List * file_list    (FileInfo *self, const char *path);
unsigned int file_exists  (FileInfo *self, const char *path);
unsigned int file_stat    (FileInfo *self, const char *path);
unsigned int file_chattr  (FileInfo *self, const char *path, unsigned int perms);
unsigned int file_mkdir   (FileInfo *self, const char *path, const char *mode);
unsigned int file_move    (FileInfo *self, const char *oldpath, const char *newpath);
unsigned int file_copy    (FileInfo *self, const char *oldpath, const char *newpath);
unsigned int file_remove  (FileInfo *self, const char *path);
unsigned int file_read_into (FileInfo *self, const char *path);
#endif
 
 
#ifdef LITE_STRING 
char *extract(String *self, const char *src, unsigned int start, unsigned int end);
char *trim(String *self, const char *str);
 
 /* Positional characters */
unsigned int posf(String *self, const char *str, const char *find, unsigned int at);
unsigned int pos(String *self, const char *str, const char *find, unsigned int at);
char *find(String *self, const char *str);
char *replace(String *self, const char *str, const char *replace, char type);
// char *concat(String *self, const char *str1, const char *str2);
char *concat(String *self, const char *cat, const char **strs);
char **isplit(String *self, const char *str, unsigned int chars);
char **csplit(String *self, const char *str, char split);
char **ssplit(String *self, const char *str, char *split);
char *reverse(String *self, const char *str);
void free(String *self);
#endif


//Random
#ifdef LITE_RANDOM 
#ifdef LITE_MT
char * rand_punct(Random *t, unsigned int length);
char * rand_numbers(Random *t, unsigned int length);
char * rand_chars(Random *t, unsigned int length);
char * rand_alnum(Random *t, unsigned int length);
char * rand_any(Random *t, unsigned int length);
int rand_number(void);
int between(Random *t, int x, int y);
int range(int x, int y);
#else
char * rand_punct (int length);
char * rand_numbers (int length);
char * rand_chars (int length);
char * rand_alnum (int length);
char * rand_any(int length);
int rand_number(void);
int between(int x, int y);
int range(int x, int y);
#endif


#ifdef LITE_EXPERIMENTAL
 #ifndef LITE_MT
void * rand_ptr(void *t, int length);
 #else
void * rand_ptr(Random *t, void *t, unsigned int length);
 #endif
#endif
void seed(void);
#endif


//Timer functions
#ifdef LITE_TIMER
void __timer_set_name (Timer *t, const char *label);

void __timer_init (Timer *t, LiteTimetype type);

void __timer_start (Timer *t
 #ifdef CV_VERBOSE_TIMER 
 , const char *file, int line
 #endif
);

void __timer_end (Timer *t
 #ifdef CV_VERBOSE_TIMER 
 , const char *file, int line
 #endif 
);

float __timer_elap (Timer *t);
void __timer_eprint (Timer *t); 
#endif


//Tokenizer functions
#ifdef LITE_TOKENIZER
#endif


//State tracker (Valgrind works so well!  Why would I want this?!)
#ifdef LITE_STATE 
extern Tracker *state;
void initialize_tracker (Tracker *lstate);
unsigned int size(void);
void *find(const char *symbol);
void *malloc(unsigned int size, const char *file, unsigned int line);
void *nalloc(unsigned int size, const char *symbol);
void free(const char *symbol);
void dump(void);
void freelast(void);
void freestate(void);
#endif


//Formatter (for formatting text and also for printing)
#ifdef LITE_FMT
#endif


//Hash tables? (This should also be where trie and something else go) 
#ifdef LITE_HASH
HashTable *ht_alloc (HashTable *h, int slots, _Bool (*cmp)(const void *, const void *)) ;
void ht_save (HashTable *h, const char *key, void *value) ;
void ht_usave (HashTable *h, const uint8_t *key, int len, void *value) ;
void *ht_get (HashTable *h, const char *key) ;
void ht_free (HashTable *h) ;
 #ifdef LITE_TEST
unsigned int _hash (HashTable *h, const char *s) ;
unsigned int _uhash (HashTable *h, const char *s) ;
 #endif
#endif


//Type inference
#ifdef LITE_INFER
#endif
#endif //LITE_H
