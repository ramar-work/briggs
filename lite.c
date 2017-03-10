/*lite.c*/
#include "lite.h"

//Memory defines
#ifdef LITE_MEM
 #define strwalk(a,b,c) \
	memwalk(a, (uint8_t *)b, (uint8_t *)c, strlen(b), strlen((char *)c))
#endif

//Socket defines
#ifdef LITE_SOCKET
 #define DEFAULT_HOST "localhost"

 #define set_sockopts(type,dom,prot) \
	sock->conntype = type, \
	sock->domain = dom, \
	sock->protocol = prot, \
	sock->hostname = (sock->server) ? ((!sock->hostname) ? DEFAULT_HOST : sock->hostname) : NULL, \
	sock->_class = (sock->server) ? 's' : 'c'

 #define domain_type(domain) \
	(domain == AF_INET) ? "ipv4" : "ipv6"

 #define type_type(type) \
		(type == SOCK_STREAM) ? "SOCK_STREAM" : \
		(type == SOCK_DGRAM) ? "SOCK_DGRAM" : \
		(type == SOCK_SEQPACKET) ? "SOCK_SEQPACKET" : \
		(type == SOCK_RAW) ? "SOCK_RAW" : \
		(type == SOCK_RDM) ? "SOCK_RDM" : "UNKNOWN SOCKET TYPE"

 #define proto_type(proto) \
	struct protoent *__mp; \
	char *tb = !(__mp=getprotobynumber(proto)) ? "unknown" : __mp->p_name; \
	endprotoent();

 #define class_type(sockclass) \
	(sockclass == 'c') ? "client" : \
	(sockclass == 'd') ? "server (child)" : "server (parent)"

 #define buffer_type(sockclass)
#endif



/* Local data structures and setters */
static Error err = { .str = NULL, .name = NULL, .file = NULL };

inline void errname (char *nam) { err.name = nam; }

inline void errfile (FILE *file) { err.file = file; }


//Errors for the entire library
enum {
	ERR_LITE_NO_ERROR,
#ifdef LITE_TIMER
	ERR_LITE_TIMER_ERROR,
#endif
#ifdef LITE_OPT
	ERR_LITE_OPT_EXPECTED_ANY,
	ERR_LITE_OPT_EXPECTED_STRING,
	ERR_LITE_OPT_EXPECTED_NUMBER,
#endif
#ifdef LITE_SOCKET
	ERR_LITE_SOCKET_EXPECTED_ANY,
	ERR_LITE_SOCKET_EXPECTED_STRING,
	ERR_LITE_SOCKET_EXPECTED_NUMBER,
#endif
#ifdef LITE_BUFFER
	ERR_LITE_BUFFER_NEW_ALLOC,
	ERR_LITE_BUFFER_ADD_ALLOC,
	ERR_LITE_BUFFER_MEM_INIT,
	ERR_LITE_BUFFER_POST_DATA,
	ERR_LITE_BUFFER_PRE_DATA,
	ERR_LITE_BUFFER_VALUE_SIZE,
#endif
#ifdef LITE_HASH
	ERR_LITE_HASH_MALLOC,  //Allocation
	ERR_LITE_HASH_SPACE,   //No more space in open addresses
#endif
};

//Error structure (for the entire library)
ErrorMap LiteErrors[] = {
	[ERR_LITE_NO_ERROR] = { 0, "No errors" },
#ifdef LITE_TIMER
	[ERR_LITE_TIMER_ERROR]        = { 0, "Timer error occurred!\n" },
#endif
#ifdef LITE_OPT
	[ERR_LITE_OPT_EXPECTED_ANY]   = { 1, "Expected argument after flag %s\n" },
	[ERR_LITE_OPT_EXPECTED_STRING]= { 1, "Expected string after flag %s\n" },
	[ERR_LITE_OPT_EXPECTED_NUMBER]= { 1, "Expected number after flag %s\n" },
#endif
#ifdef LITE_SOCKET
#endif
#ifdef LITE_BUFFER
	/*memcpy is probably what was causing the crashes*/
	[ERR_LITE_BUFFER_NEW_ALLOC]   = { 0,"Failed to allocate bytes for new buffer."},
	[ERR_LITE_BUFFER_ADD_ALLOC]   = { 0,"Failed to allocate additional bytes for buffer."},
	[ERR_LITE_BUFFER_MEM_INIT]    = { 0,"Failed to initialize memory."},
	[ERR_LITE_BUFFER_POST_DATA]   = { 0,"Failed to append to data block."},
	[ERR_LITE_BUFFER_PRE_DATA]    = { 0,"Failed to prepend to data block."},
	[ERR_LITE_BUFFER_VALUE_SIZE]  = { 0,"Chunk too large for self."},
#endif
};


#ifdef LITE_PRINTF
//Many libraries in lite ship with functions to print their native data structures.
//Let's get rid of these via define
#ifdef LITE_SOCKET
void socket_printf (Socket *sock) {
	stprintf("class",       class_type(sock->_class));
	stprintf("conntype",    type_type(sock->conntype));
	stprintf("opened",      sock->opened ? "yes" : "no"); 
	stprintf("domain",      domain_type(sock->domain));
	nmprintf("buffer size", sock->bufsz);
	nmprintf("backlog",     sock->backlog);
	stprintf("hostname",    sock->hostname);
	nmprintf("fd",          sock->fd);
	nmprintf("port",        sock->port);
	#if 0
	fprintf(stderr, "tcp socket buffer: %p (%s)\n", (void *)sock->buffer,
		sock->_class == 'c' ? "client" : sock->_class == 'd' ? "child" : "server");
	#endif
}
#endif
#endif

inline static void 
errprintf (char *msg)
{
	/* This needs to eat more arguments */
	fprintf(!err.file ? stderr : err.file, "%s: %s\n",
		!err.name ? "(anonymous program)" : err.name, msg); 
}


/* Codes to be used with vvprintf */
#if VVPRINTF 
inline void * 
erritemf (char *file, int line, void *p) 
{
	fprintf(stderr, STAT_STR, file, line);
	fprintf(stderr, "%s\n", !p? "FAIL!" : "SUCCESS!");
	return p;
}


inline int
errstatusf (char *file, int line, int code)
{
	fprintf(stderr, STAT_STR, file, line);
	fprintf(stderr, "%s\n", !code ? "FAIL!" : "SUCCESS!");
	return code;
}
#else
#endif


/* One-line returns cannot be done with macros, so we resort to this... */
#if VVPRINTF
 inline void *__errnull (char *file, int line, char *msg) {
	fprintf(stderr, STAT_STR, file, line);
#else
 inline void *__errnull (char *msg) {
#endif
	errprintf(msg);
	return NULL;
}


#if VVPRINTF
 inline void __errstep (char *file, int line, char *msg) {
	fprintf(stderr, STAT_STR, file, line);
#else
 inline void __errstep (char *msg) {
#endif
	errprintf(msg);
	getchar();
}


#if VVPRINTF
 inline int __errstat (char *file, int line, int code, char *msg) {
	fprintf(stderr, STAT_STR, file, line);
#else
 inline int __errstat (int code, char *msg) {
#endif
	errprintf(msg);
	return code;
}

#if VVPRINTF
 inline void __errexit (char *file, int line, int code, char *msg) {
	fprintf(stderr, STAT_STR, file, line);
#else
 inline void __errexit (int code, char *msg) {
#endif
	errprintf(msg);
	exit(code);
}


#if VVPRINTF
 inline int __errfree (char *file, int line, int code, void *p, char *msg) {
	fprintf(stderr, STAT_STR, file, line);
#else
inline int __errfree (int code, void *p, char *msg) {
#endif
	free(!p ? NULL : p);
	return code;
}


#if VVPRINTF
 inline int __errsys (char *file, int line, char *msg) {
	fprintf(stderr, STAT_STR, file, line);
#else
 inline int __errsys (char *msg) {
#endif
	char buf[1024] = { 0 };
	if (msg)
		snprintf(buf, 1023, "%s - %s", strerror(errno), msg);
	else
		snprintf(buf, 1023, "%s", strerror(errno));
	errprintf(buf);
	return 0;
}



#ifdef LITE_TIMER 
static const unsigned long LITE_QUANT_1T = 1000;
static const unsigned long LITE_QUANT_1M = 1000000;
static const unsigned long LITE_QUANT_1B = 1000000000;

//Initiailize a timer
void __timer_init (Timer *t, LiteTimetype type) {
	memset(t, 0, sizeof(Timer));	
	t->type = type;
}


//Set the name of a timer
void __timer_set_name (Timer *t, const char *label) {
	t->label = label;
}


//Start a currently running timer
 #ifndef CV_VERBOSE_TIMER 
void __timer_start (Timer *t)
{
 #else
void __timer_start (Timer *t, const char *file, int line)
{
	t->file = file;
	t->linestart = line;
 #endif 
	t->clockid = CLOCK_REALTIME;
	clock_gettime( t->clockid, &t->start );
}


//Stop a currently running timer
 #ifndef CV_VERBOSE_TIMER 
void __timer_end (Timer *t)
{
 #else
void __timer_end (Timer *t, const char *file, int line)
{
	t->file = file;
	t->lineend = line;
 #endif 
	clock_gettime( t->clockid, &t->end );
}


//Returns difference of start and end time
float __timer_elap (Timer *t) {
	//Define stuff
	unsigned long nsdiff = 0;
	time_t secs = 0;

	//Get the raw elapsed seconds and nanoseconds
	if ((secs = t->end.tv_sec - t->start.tv_sec) == 0)
		nsdiff = t->end.tv_nsec - t->start.tv_nsec;
	else if (secs < 0) {
		fprintf(stderr, "Timer error occurred!\n");
		return -1; /*Some kind of error occurred*/
	}
	else if (secs > 0) {
		if ((nsdiff = (LITE_QUANT_1B - t->start.tv_nsec) + t->end.tv_nsec) > LITE_QUANT_1B) {
			nsdiff -= LITE_QUANT_1B;
			secs += 1;
		}
	}


	//Choose a modifier and make any final calculations for formatting
	switch (t->type) {
		case LITE_NSEC:   //0.000,000,001
			return (float)nsdiff;
		case LITE_USEC:   //0.000,001
			return ((float)secs * LITE_QUANT_1M) + (((float)nsdiff / (float)LITE_QUANT_1B) * (float)LITE_QUANT_1M);
		case LITE_MSEC:   //0.001
			return ((float)secs * LITE_QUANT_1T) + (((float)nsdiff / (float)LITE_QUANT_1B) * (float)LITE_QUANT_1T);
		case LITE_SEC:
			return ((float)secs + ((float)nsdiff / (float)LITE_QUANT_1B));
		default:
			return -1;
	}
}


//Pretty prints difference in requested format of start and end time
void __timer_eprint (Timer *t) {
	//Define some stuff...
	unsigned long nsdiff = 0;
	time_t secs = 0;
	double mod = 0;
	char ch[64] = { 0 };
	const char *ts ;
	const char *time   = "ns\0ms\0us\0s";
	const char *label  = (t->label) ? t->label :
	 #ifdef CV_VERBOSE_TIMER
		(t->file) ? t->file : "anonymous"
	 #else
		"anonymous"
	 #endif
	;
	const char *fmt    =
   #ifdef CV_VERBOSE_TIMER
	  //"routine @[ %-20s %d - %d ] completed in %11ld %s\n"
	  "routine @[ %-20s %d - %d ] completed in %s %s\n"
   #else
	  "routine [ %-20s ] completed in %s %s\n"
   #endif
	;


	//Get the raw elapsed seconds and nanoseconds
	if ((secs = t->end.tv_sec - t->start.tv_sec) == 0)
		nsdiff = t->end.tv_nsec - t->start.tv_nsec;
	else if (secs < 0) {
		fprintf(stderr, "Timer error occurred!\n");
		return; /*Some kind of error occurred*/
	}
	else if (secs > 0) {
		if ((nsdiff = (LITE_QUANT_1B - t->start.tv_nsec) + t->end.tv_nsec) > LITE_QUANT_1B) {
			nsdiff -= LITE_QUANT_1B;
			secs += 1;
		}
	}


	//Choose a modifier and make any final calculations for formatting
	switch (t->type) {
		case LITE_NSEC:   //0.000,000,001
			snprintf( ch, 64,  "%ld", nsdiff );
			ts = &time[0];
			break;
		case LITE_USEC:   //0.000,001
			mod = ((float)secs * LITE_QUANT_1M) + (((float)nsdiff / (float)LITE_QUANT_1B) * (float)LITE_QUANT_1M);
			ts = &time[6];
			snprintf( ch, 64, "%.6f", mod);
			break;
		case LITE_MSEC:   //0.001
			mod = ((float)secs * LITE_QUANT_1T) + (((float)nsdiff / (float)LITE_QUANT_1B) * (float)LITE_QUANT_1T);
			ts = &time[3];
			snprintf( ch, 64, "%.6f", mod);
			break;
		case LITE_SEC:
			mod = ((float)secs + ((float)nsdiff / (float)LITE_QUANT_1B));
			ts = &time[9];
			snprintf( ch, 64, "%.6f", mod);
			break;
		default:
			return;
	}

	fprintf(stderr, fmt, label, ch, ts); 
}
#endif




#ifdef LITE_BUFFER
//Buffer stuff
Buffer * buffer_init (uint32_t size) {
	Buffer *tmp = NULL;
	return (tmp = malloc(size));
}


void buffer_print (Buffer *self) {
	uint8_t *p = self->body;
	if (p)
		for (int g=0;g<self->size;g++)
			fprintf(stderr, "%c", p[g]);
	 #if 0 
		int g;
		char *p = (char *)b;
		fprintf(stderr, "%s\n", !msg ? "Buffer contents:": msg);
		for (g=0;g<length;g++)
			fprintf(stderr, "%c", p[g]);
		fprintf(stderr, "Message was %d bytes long.\n", g);
	 #endif
}

#if 0
//Add to and return a buffer
static uint8_t *allocate_buffer (uint8_t *old, uint8_t *new, uint32_t size) {
	if (!old) {
		if (!(old = malloc(size))) //
			return errnull("Failed to allocate bytes for new buffer.");
	}
	else if ((new = realloc(old, size)) == NULL) {
		return errnull("Failed to allocate additional bytes for buffer.");
	}
	return new;
}
#endif


//Free a buffer
void buffer_free (Buffer *self) {
	if (self->body) 
		free(self->body);
	free(self);
}



//Append to a buffer
_Bool buffer_append (Buffer *self, void *body, unsigned int size)
{
	vvprintf("Appending %d bytes to current buffer %p.", size, (void *)self);
	uint32_t p = self->pos;
	uint8_t *nbody=NULL;

	#if 1  /*Use 'allocate_buffer'*/
	if (!self->body) {
		if (!(self->body = malloc(p + size)))
			return errstat(1, "Failed to allocate bytes for new buffer.");
	}
	else if ((nbody = realloc(self->body, p + size)) == NULL) {
		return errstat(1, "Failed to allocate additional bytes for buffer.");
	}
	#endif
	
	if (memset(&nbody[p], 0, size) == NULL)
		return errstat(1, "Failed to initialize memory.");

	if (memcpy(&nbody[p], body, size) == NULL)
		return errstat(1, "Failed to append to data block.");

	/* A final update */
	self->pos = self->size += size;
	self->body = nbody;

	vvprintf("\tCurrent buffer size: %d.", self->size);
	return SUCCESS;
}



_Bool buffer_prepend (Buffer *self, void *body, unsigned int size)
{
	vvprintf("Prepending %d bytes to current buffer %p.", size, (void *)self);
	#define db(s, i, e) \
		fprintf(stderr, "%s\n%s\n", s, "======================"); \
		vvprintf("Current size is %d", e); \
		for (c=i;c<e;c++) \
			fprintf(stderr, "%d: %c\n", c, nbody[c]); \
		pprintf("...")


	/*Define*/
	//buffer_t *buffer = self->data;
	uint32_t pos = self->pos;
	uint8_t *nbody = NULL;
	/* Move the original size to the end. */
	int32_t ss = pos - size;
	int32_t p = pos;
	int32_t z = size;

	/* Try a realloc */
	#if 1  /*Use 'allocate_buffer'*/
	if (!self->body) {
		if (!(self->body = malloc(pos + size)))
			return errstat(1, "Failed to allocate bytes for new buffer.");
	}
	else if ((nbody = realloc(self->body, pos + size)) == NULL) {
		return errstat(1, "Failed to allocate additional bytes for buffer.");
	}
	#endif

	/* memset the area after... */
	if (memset(&nbody[pos], 0, size) == NULL)
		return errstat(1, "Failed to initialize memory.");

	/* Move everything up in this ridiculous fashion, b/c memmove() fucking sucks! */
	while (p > 0) {
		if (memmove(&nbody[p], &nbody[ss], z) == NULL)
			return errstat(1, "Failed to prepend to data block.");
		if (memset(&nbody[ss], 0, size) == NULL)
			return errstat(1, "Failed to initialize memory.");
		p -= size;
		ss -= size;

		if (ss < 0) {
			ss = 0;
			if (memmove(&nbody[z], &nbody[ss], p) == NULL)
				return errstat(1, "Failed to prepend to data block.");
			if (memset(&nbody[ss], 0, z) == NULL)
				return errstat(1, "Failed to initialize memory.");
			break;
		}
	}

	/* Prepend the body. */
	if (memcpy(&nbody[0], body, size) == NULL)
		return errstat(1, "Failed to prepend to data block.");

	/* A final update */
	self->pos = self->size += size;
	self->body = nbody;
	return SUCCESS; 
}



_Bool buffer_insert (Buffer *self, void *body, unsigned int size)
{
	vvprintf("Adding %d bytes into buffer at %p", size, (void *) self);
	if ((self->pos + size) > self->size)
		return errstat(0, "Chunk too large for self.");

	if (memcpy(&self->body[self->pos], body, size) == NULL)
		return errstat(0, "Could not append to string.");

	self->pos += size;
	return SUCCESS; 
}



#if 0
/* Read a file or string into a buffer. */
Buffer *buffer_read (Buffer *self, void *p, unsigned int tsize, unsigned int bsize)
{
	FILE *fh = (FILE *)p;
	int b, c = 0, buf = (!bsize) ? 1024 : bsize;
	unsigned char mybuf[(const int)buf];
	buffer_t *data = self->data;

	/* You shouldn't have to, but it might be good to get the filesize */
	while ((b = fgetc(fh)) != EOF) {
		mybuf[c] = b;
		c++;
		if (c == buf) {
			if (!self->append(self, &mybuf, c)) return NULL;
			reseti(c);
		}
	}

	/* Finalize this */
	return (!self->append(self, &mybuf, c)) ? NULL : self->data;
}




/* ... */
_Bool buffer_write (Buffer *self, char *filename, unsigned int s, unsigned int e)
{
	int fd; 
	ssize_t wb = 0;
	/* BufferData *data = self->data; */

	if ((fd = open(filename, O_RDONLY)) < 0)
		return errstat(0, "Failed to open file at filename.");

	wb = write(fd, &self->data->body[s], !e ? self->data->size : (e - s));
	return (wb < (e - s)) ? 0 : 1;
}
 #endif
#endif /*NOBUFFER*/




#ifndef LITE_LIST 
//Prepend to a list
_Bool list_prepend (List *self, void *item) 
{
	//List *list = self; 
	ListNode *node; 

	if (!(node = nalloc(sizeof(ListNode), NULL))) 
		return 0;
	if (!(node->item = nalloc(self->element_size, NULL))) 
		return 0;
	memcpy(node->item, item, self->element_size);
	node->next = self->head;
	self->head = node;

	if (!self->tail)
		self->tail = self->head;

	self->count++; 
	return 1;
}


//Append to a list
_Bool list_append (List *self, void *item) 
{
	//list_t *list = self->self; 
	ListNode *node; 

	if (!(node = nalloc(sizeof(ListNode), NULL))) return 0;
	if (!(node->item = nalloc(self->element_size, NULL))) return 0;
	node->next = NULL;
	memcpy(node->item, item, self->element_size);

	if (!self->count)
		self->head = self->tail = node;
	else {
		self->tail->next = node; // Why is this?
		self->tail = node;
	}

	self->count++; 
	return 1;
}


//Count the items in a list
uint32_t list_count (List *self) { 
	return self->count; 
}



//Get the size of a list
uint32_t list_size (List *self) { 
	return (self->count * self->element_size); 
}



//Free an allocated list
void list_free (List *self) {
	ListNode *node;
	 
	while (self->head != NULL) {
		node = self->head;
		self->head = node->next;	
		if (self->free) 
			self->free(node->item);
		free(node->item);
		free(node);
	}

	free(self);
}



//Get the next item in a list
ListNode *list_next (List *self) {
	return self->curr = (!self->curr) ? self->head : self->curr->next;
}



//Create a new list
List *list_new (uint32_t size, void (*free_f)(void *self)) {
	List *list = nalloc(sizeof(List), NULL);
	list->count=0;
	list->element_size = size;
	list->head = list->tail = list->curr = NULL;
	list->free = !free_f ? NULL : free_f;
	return list;
}
#endif




#ifdef LITE_SOCKET
//Get the address information of a socket.
void socket_addrinfo (Socket *sock)
{
	fprintf(stderr, "%s\n", "Getting address information for socket.");

	struct addrinfo *peek; 
	struct sockaddr_in *ipv4;
	struct sockaddr_in6 *ipv6;
	int status = getaddrinfo(sock->hostname, sock->portstr, &sock->hints, &sock->res);
	if (status != 0)
		errexit(1, "Could not get address information for this socket.");

	/* Loop through each */
	for (peek = sock->res; peek != NULL; peek = peek->ai_next) {
		void *addr;	
		char *ipver;
		if (peek->ai_family == AF_INET) {
			ipv4 = (struct sockaddr_in *)peek->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		}
		else {
			ipv6 = (struct sockaddr_in6 *)peek->ai_addr;
			addr = &(ipv6->sin6_addr);
			ipver = "IPv4";
		}

		if (inet_ntop(peek->ai_family, addr, sock->address, sizeof(sock->address)) == NULL)
			continue;
	
		fprintf(stderr, "%s: %s\n", ipver, sock->address);
		/* Break somewhere after finding a valid address. */
		break;
	}

	/* Free this */
	freeaddrinfo(peek);

	/* A list will probably be used here */
	// OBJECT(list) *addresses
	// { .ipver = 4, ipstr = 192.10..., .ipbin = peek->ai_addr, .len = peek->ai_addrlen }
}



//initialize a socket...
_Bool socket_open (Socket *sock) {
	/* All of this can be done with a macro */
	sock->addrsize = sizeof(struct sockaddr);
	sock->bufsz = !sock->bufsz ? 1024 : sock->bufsz;
	sock->opened = 0;
	sock->backlog = 500;
	sock->waittime = 5000;  // 3000 microseconds

	if (!sock->proto || strcasecmp(sock->proto, "tcp") == 0)
		set_sockopts(SOCK_STREAM, PF_INET, IPPROTO_TCP);
	else if (strcasecmp(sock->proto, "udp") == 0)
		set_sockopts(SOCK_DGRAM, PF_INET, IPPROTO_UDP);

	/* Check port number (clients are zero until a request is made) */
	if (!sock->port || sock->port < 0 || sock->port > 65536) {
		/* Free allocated socket */
		vvprintf("Invalid port specified.");
		return 0;
		//return errnull("Invalid port specified.");
	}


	/* Set up the address data structure for use as either client or server */	
	if (sock->_class == 's') 
	{
		/*there must be a way to do this WITHOUT malloc*/
		//if ((sock->srvaddrinfo = (struct sockaddr_in *)nalloc(sizeof(struct sockaddr_in), "sockaddr.info")) == NULL)
		//	return;
			//return errnull("Could not allocate structure specified.");

		/*This ought to work...*/
		sock->srvaddrinfo = &sock->tmpaddrinfo;
		memset(sock->srvaddrinfo, 0, sizeof(struct sockaddr_in));
		struct sockaddr_in *saa = sock->srvaddrinfo; 
		saa->sin_family = AF_INET;
		saa->sin_port = htons(sock->port);
		(&saa->sin_addr)->s_addr = htonl(INADDR_ANY);
	}
	else if (sock->_class == 'c') 
	{
		/* Set up the addrinfo structure for a future client request. */
		struct addrinfo *h; 
		memset(&sock->hints, 0, sizeof(sock->hints));
		h = &sock->hints;
		h->ai_family = sock->domain;
		h->ai_socktype = sock->conntype;
	}

	/* Finally, create a socket. */
	if ((sock->fd = socket(sock->domain, sock->conntype, sock->protocol)) == -1) {
		return 0;
	}
	sock->opened = 1;

	/* Set timeout, reusable bit and any other options */
	struct timespec to = { .tv_sec = 2 };
	if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, &to, sizeof(to)) == -1) {
		// sock->free(sock);
		sock->err = errno;
		errsys("Could not reopen socket.");
		return 0;
	}
	return 1;
}



//Bind to a socket
_Bool socket_bind (Socket *sock) {
	//set errno
	return (bind(sock->fd, (struct sockaddr *)sock->srvaddrinfo, sizeof(struct sockaddr_in)) != -1);
//		return errsys("Could not bind to socket.");
}



//Listen for connections over a socket.
_Bool socket_listen (Socket *sock)
{
	//set errno
	return (listen(sock->fd, sock->backlog) != -1);
		//return errsys("Could not listen out on socket.");
}



//Open a socket for UDP
_Bool socket_udp_recv (Socket *self)  {
#if 0
		case SOCK_DGRAM:
			rcvd = recvfrom(sock->fd, msg, ws, 0,
				NULL, NULL);
				//sock->cliaddr, &sock->cliaddrlen);

			if (rcvd == -1) return 0;
			msg[rcvd] = 0;
			fprintf(stderr, "udp recv'd bytes: %d\n", rcvd);
			fprintf(stderr, "%s\n", msg);	

			while (1) {
				rcvd = recvfrom(sock->fd, msg, ws, 0,
					NULL, NULL);
				fprintf(stderr, "udp recv'd bytes: %d\n", rcvd);
				fprintf(stderr, "%s\n", msg);	

				if (rcvd == -1)
					return 0;  // return false and sock->error = errno;
				if (rcvd < ws)
					break;	
			}
			return 1;
#endif
	return 0;
}



//Opens a non blocking socket.
//This function is not a good idea for select I don't think...
_Bool socket_tcp_recv (Socket *sock, uint8_t *msg, uint32_t *len) {
	int32_t size=0, rcvd=0, ws=sock->bufsz;

	//If it's -1, die.  If it's less than buffer, die
	while (1) {
		size += rcvd = recv(sock->fd, &msg[size], ws, 0);
fprintf(stderr, "rcvd: %d\n", rcvd);
		//Error occurred, free or reset the buffer and die
		if (rcvd == -1) {
			//handle recv() errors...
			sock->err = errno;
			return errsys("recv() error occurred");
		}
		//End of message reached before end of buffer
		else if (rcvd < ws) {	
			*len = size;
			return 1;
		}
	}

	//Only an outside event (like reaching the -- on a 
	//mutipart request) can really signal that we're done.
	*len = size;
	return 1;
}



#if 0
_Bool socket_udp_send (Socket *sock, uint8_t *msg, uint32_t len) {
	int bs=0; 
#if 0
	bs = sendto(sock->fd, msg, msglen, 0, 
		(struct sockaddr *)sock->srvaddrinfo, 
		sizeof(struct sockaddr_in)); 
	fprintf(stderr, "Size of message to be sent: %d\n", msglen);
	fprintf(stderr, "Message to be sent:\n %s\n", msg);
	fprintf(stderr, "(press enter to continue...)\n");
	getchar();	
#endif
	bs = sendto(sock->fd, msg, len, 0, 
		(struct sockaddr *)sock->cliaddrinfo, 
		sizeof(sock->cliaddr)); 


	if (bs == -1)
		return 0;

	fprintf(stderr, "Sent UDP bytes: %d\n", bs);
	fprintf(stderr, "Sent message:   %s\n", msg);
#if 0
	while (1) {
		if (bs==msglen)
			break;

		if (bs == -1)
			return 0;
	
		fprintf(stderr, "Sent UDP bytes: %d\n", bs);
		pprintf("..>");
	}
#endif
	return 1;
}
#endif



#if 0
_Bool socket_tcp_send (Socket *sock, uint8_t *msg, uint32_t len) {
	/* What is going on? */
	int bs = 0;

#if 0
	fprintf(stderr, "Attempting to send message over fd '%d'\n", sock->fd);
	fprintf(stderr, "message contents:\n");
		
	int c = 0;
	// chardump(msg, ws > rcvd ? rcvd : ws);
	for (c=0;c<msglen;c++)
		fprintf(stderr, "'%c' ", msg[c]);
#endif
	while (1) { 
		bs = send(sock->fd, msg, len, 0);
		// usleep(sock->waittime);
		if (bs==len)
			break;
		if (bs == -1)
			return 0;
		// keep trying
	}
	return SUCCESS;
}
#endif



//Accept connections
_Bool socket_accept (Socket *sock, Socket *new) {
	/* Clone current socket data */
	if (!memcpy(new, sock, sizeof(Socket))) {
		fprintf(stderr, "Could not copy original parent socket data.\n");
		return 0;
	}

	/* Accept a connection. */	
	if ((new->fd = accept(sock->fd, NULL, NULL)) == -1) 
		return ((sock->err = errno) ? 0 : 0);
		
	/* Set socket description */
	new->_class = 'd';
	return 1;
}



//Send data via UDP
_Bool socket_udp_send (Socket *sock, uint8_t *msg, uint32_t length) {
	//int msglen = (!length) ? strlen(msg) : length;	
	int32_t bs = 0;
#if 0
	bs = sendto(sock->fd, msg, msglen, 0, 
		(struct sockaddr *)sock->srvaddrinfo, 
		sizeof(struct sockaddr_in)); 
	fprintf(stderr, "Size of message to be sent: %d\n", msglen);
	fprintf(stderr, "Message to be sent:\n %s\n", msg);
	fprintf(stderr, "(press enter to continue...)\n");
	getchar();	
#else
	bs = sendto(sock->fd, msg, length, 0, 
		(struct sockaddr *)sock->cliaddrinfo, 
		sizeof(sock->cliaddr)); 
#endif

	if (bs == -1) {
		sock->err = errno;
		return 0;
	}

	fprintf(stderr, "Sent UDP bytes: %d\n", bs);
	fprintf(stderr, "Sent message:   %s\n", msg);
#if 0
	while (1) {
		if (bs==msglen) break;
		if (bs == -1) return 0;
		fprintf(stderr, "Sent UDP bytes: %d\n", bs);
	}
#endif
	return 1;
}



#if 0
//Parse URI
_Bool socket_parse_uri (URIData *u, const char *uri) {
	int p, len=strlen(uri);
	//Pack protocol
	if (!memstr(uri, "://", len)) return 0;
	memcpy(u->protocol, 0, (p = memstrat(uri, "://", len)));
	//Check for a port (that's the end of host)
	if (!memchr(&uri[p+3], '/', len - (p+3)))
		if ((p = memchrat(&uri[p+3], ':', len - (p+3)))) {
		//Pack hostname if you can find it
	else {

	}	

	//Pack port
	return 1;
}
#endif


//Get address info
_Bool socket_getaddrinfo (Socket *self) {
	return 0;
}



#if 0
_Bool socket_connect (Socket *self, const char *uri, unsigned int port) {
	int stat;
	char *ps;
	uri_t *h;
	struct addrinfo *r;
	socket_t *s = self->data;
	OBJECT(urn) *urn = NEW(urn)(NULL);

	/* Parse the string */
	if (!urn->parse(urn, (!uri ? s->hostname : uri), port))
		return 0;

	/* Information */
	h = urn->parsed(urn); 
	urn->info(urn);

	/* Debuggable dump */
	fprintf(stderr, "protocol:       %s\n", h->protocol);
	fprintf(stderr, "hostname:       %s\n", h->name);
	fprintf(stderr, "resource:       %s\n", h->resource);
	fprintf(stderr, "username:       %s\n", h->username);
	fprintf(stderr, "password:       %s\n", h->password);
	fprintf(stderr, "port:           %d\n", h->port ? h->port : 0);
	fprintf(stderr, "port (str):     %s\n", h->sport);

	/* Finally, get address information and ... */
	// sock->error = gai_strerror(stat)); 
	if ((stat = getaddrinfo(h->name, h->sport, &s->hints, &s->res)) != 0)
		errexit(1, (char *)gai_strerror(stat));

	/* ... connect! */
	// sock->error = "connection error..."
	// r = s->res;
	if (connect(s->fd, s->res->ai_addr, s->res->ai_addrlen) == -1)
		errexit(1, strerror(errno));	

	/* Set this for later calls */
	self->data->urn = urn;
	return 1;
}
#endif



//Send data via TCP Socket
_Bool socket_tcp_send (Socket *sock, uint8_t *msg, uint32_t length) {
	int32_t bs = 0;

#if 0
	fprintf(stderr, "Attempting to send message over fd '%d'\n", sock->fd);
	fprintf(stderr, "message contents:\n");
		
	int c = 0;
	// chardump(msg, ws > rcvd ? rcvd : ws);
	for (c=0;c<msglen;c++)
		fprintf(stderr, "'%c' ", msg[c]);
#endif
		
	while (1) { 
		bs = send(sock->fd, msg, length, 0);
		// usleep(sock->waittime);
		if (bs==length)
			break;
		if (bs == -1)
			return 0;
	}
	return SUCCESS;
}
#endif



#ifdef LITE_MEM 
/*Compare to uint8_t's*/
_Bool memstr (const void * a, const void *b, int size) {
	int32_t ct=0, len = strlen((const char *)b);
	const uint8_t *aa = (uint8_t *)a;
	const uint8_t *bb = (uint8_t *)b;
	_Bool stop=1;
	while (stop) {
		while ((stop = (ct < (size - len))) && memcmp(aa + ct, bb, 1) != 0) { 
			//fprintf(stderr, "%c", aa[ct]);
			ct++; continue; }
		if (memcmp(aa + ct, bb, len) == 0)
			return 1;	
		ct++;
	}
	return 0;	
}

//Return count of occurences of a character in some block.
int32_t memchrocc (const void *a, const char b, int32_t size) {
	_Bool stop=1;
	int32_t ct=0, occ=-1;
	uint8_t *aa = (uint8_t *)a;
	char bb[1] = { b };
	while (stop) {
		occ++;
		while ((stop = (ct < size)) && memcmp(aa + ct, bb, 1) != 0) ct++;
		ct++;
	}
	return occ;
}


//Return count of occurences of a string in some block.
int32_t memstrocc (const void *a, const void *b, int32_t size) {
	_Bool stop=1;
	int32_t ct=0, occ=0;
	uint8_t *aa = (uint8_t *)a;
	uint8_t *bb = (uint8_t *)b;
	int len     = strlen((char *)b);
	while (stop) {
		while ((stop = (ct < (size - len))) && memcmp(aa + ct, bb, 1) != 0) ct++;
		if (memcmp(aa + ct, bb, len) == 0) occ++;
		ct++;
	}
	return occ;
}



//Initialize a block of memory
_Bool memwalk (Mem *mm, uint8_t *data, uint8_t *tokens, int datalen, int toklen) {
#if 0
fprintf(stderr, "Inside memwalk: ");
write(2, data, datalen);
write(2, "\n", 1);
#endif
	int rc    = 0;
	mm->pos   = mm->next;
	mm->size  = memtok(&data[mm->pos], tokens, datalen - (mm->next - 1), toklen);
	if (mm->size == -1) {
	 mm->size = datalen - mm->next;
	}
	mm->next += mm->size + 1;
	//rc      = ((mm->size > -1) && (mm->pos <= datalen));
	rc        = (mm->size > -1);
	mm->chr   = !rc ? 0 : data[mm->next - 1];
	mm->pos  += mm->it;
	mm->size -= mm->it;
#if 0
fprintf(stderr, "rc: %d\n", rc);
fprintf(stderr, "datalen: %d\n", datalen);
fprintf(stderr, "mm->pos: %d\n", mm->pos);
fprintf(stderr, "mm->size: %d\n", mm->size);
#endif
	return rc; 
}


//Where exactly is a substr in memory
int32_t memstrat (const void *a, const void *b, int32_t size) {
	_Bool stop=1;
	int32_t ct=0;//, occ=0;
	uint8_t *aa = (uint8_t *)a;
	uint8_t *bb = (uint8_t *)b;
	int len     = strlen((char *)b);
	//while (stop = (ct < (size - len)) && memcmp(aa + ct, bb, len) != 0) ct++; 
	while (stop) {
		while ((stop = (ct < (size - len))) && memcmp(aa + ct, bb, 1) != 0) ct++;
		if (memcmp(aa + ct, bb, len) == 0)
			return ct; 
		ct++;
	}
	return -1;
}

//Where exactly is a substr in memory
int32_t memchrat (const void *a, const char b, int32_t size) {
	_Bool stop=1;
	int32_t ct=0;// occ=0;
	uint8_t *aa = (uint8_t *)a;
	//uint8_t *bb = (uint8_t *)b;
	char bb[1] = { b };
	//while (stop = (ct < (size - len)) && memcmp(aa + ct, bb, len) != 0) ct++; 
	while ((stop = (ct < size)) && memcmp(aa + ct, bb, 1) != 0) ct++;
	return (ct == size) ? -1 : ct;
}


//Finds the 1st occurence of one char, Keep running until no tokens are found in range...
int32_t memtok (const void *a, const uint8_t *tokens, int32_t sz, int32_t tsz) {
	int32_t p=-1,n;
	
	for (int i=0; i<tsz; i++)
	#if 1
		p = ((p > (n = memchrat(a, tokens[i], sz)) && n > -1) || p == -1) ? n : p;
	#else
	{
		p = ((p > (n = memchrat(a, tokens[i], sz)) && n > -1) || p == -1) ? n : p;
		fprintf(stderr, "found char %d at %d\n", tokens[i], memchrat(a, tokens[i], sz));
		nmprintf("p is", p);
	}
	#endif
	
	return p;
}


//Finds the first occurrence of a complete token (usually a string). 
//keep running until no more tokens are found.
int32_t memmatch (const void *a, const char *tokens, int32_t sz, char delim) {
	int32_t p=-1, n, occ = -1;

	/*Check that the user has supplied a delimiter. (or fail in the future)*/
	if (!(occ = memchrocc(tokens, delim, strlen(tokens))))
		return -1 /*I found nothing, sorry*/;

	/*Initialize a temporary buffer for each copy*/
	int t = 0; 
	char buf[strlen(tokens) - occ];
	memset(&buf, 0, strlen(tokens) - occ);

	/*Loop through each string in the token list*/
	while (t < strlen(tokens) && (n = memtok(&tokens[t], (uint8_t *)"|\0", sz, 2)) > -1) {
		/*Copy to an empty buffer*/
		memcpy(buf, &tokens[t], n);
		buf[n] = '\0';
		t += n + 1;

		/*This should find the FIRST occurrence of what we're looking for within block*/
		p = ((p > (n = memstrat(a, buf, sz)) && n > -1) || p == -1) ? n : p;
		/*fprintf(stderr, "found str %s at %d\n", buf, memstrat(a, buf, sz)); nmprintf("p is", p);*/
		memset(&buf, 0, strlen(tokens) - occ);
	}
	return p;
}


/*Copy strings*/
char *memstrcpy (char *dest, const uint8_t *src, int32_t len) {
	memcpy(dest, src, len);
	dest[len]='\0';
	return dest;
}
#endif



#ifdef LITE_OPT
//Set values when the user asks for them
static _Bool opt_set_value (char **av, Value *v, char type, char *err) {
	/*Get original flag and other things.*/
	char flag[64]={0}; 
	snprintf(flag, 63, "%s", *av);
	av++;

	/*Catch what may be a flag*/
	if (!*av || (strlen(*av) > 1 && *av[0] == '-' && *av[1] == '-'))
		return (snprintf(err, 1023, "Expected argument after flag %s\n", flag) && 0); 
	
	/*Evaluate the three different types*/
	if (type == 'c') {
		v->c = *av[0];	
	}
	else if (type == 's') {
		_Bool isstr=0;
		for (int i=0;i<strlen(*av); i++) { 
			/*We can safely assume this is an ascii string, if this passes*/
			if ((*av[i] > 32 && *av[i] < 48) || (*av[i] > 57 && *av[i] < 127)) {
				isstr = 1;
				break;
			}
		}

		if (!isstr)
			return (snprintf(err, 1023, "Expected string after flag %s\n", flag) && 0);
		v->s = *av;	
	}
	else if (type == 'n') {
		char *a = *av; /*Crashes for some reason if I just use dereference*/
		for (int i=0;i<strlen(a); i++)
			if ((int)a[i] < 48 || (int)a[i] > 57) /*Not a number check*/
				return (snprintf(err, 1023, "Expected number after flag %s\n", flag) && 0);
		v->n = atoi(*av);	
	}

	return 1; 
}


//Dump all options and show a usage message.
_Bool opt_usage (Option *opts, const char *prog, const char *msg, int status) {
	if (prog && msg)
		fprintf(stderr, "%s: %s\n", prog, msg);
	else if (msg)
		fprintf(stderr, "%s\n", msg);
	while (!opts->sentinel) { 
		if (opts->type != 'n' && opts->type != 'c' && opts->type != 's') 
			fprintf(stderr, "%-2s, %-20s %s\n", opts->sht ? opts->sht : " " , opts->lng, opts->description);
		else {
			char argn[1024]; memset(&argn, 0, 1024);
			snprintf(argn, 1023, "%s <arg>", opts->lng);
			fprintf(stderr, "%-2s, %-20s %s\n", opts->sht ? opts->sht : " " , argn, opts->description);
		}
		opts++;
	}
	exit(status);
	return 1;
}


//Check if an option was set by a user
_Bool opt_set (Option *opts, char *flag) {
	Option *o = opts;
	while (!o->sentinel) {
		if (strcmp(o->lng, flag) == 0 && o->set)
			return 1; 
		o++;
	}
	return 0;
}


//Return a value if it was set, or a nil value (NULL for strings, 0 for numbers)
Value opt_get (Option *opts, char *flag) {
	Option *o = opts;
	while (!o->sentinel) {
		if (strcmp(o->lng, flag) == 0)
			return o->v; 
		o++;
	}
	return o->v; /*Should be the last value, and it should be blank*/
}



//Evaluate options that the user gave and die with a message
_Bool opt_eval (Option *opts, int argc, char **av) {
	/*Evaulate options*/
	char buf[1024]={0};
	while (*av) {
		Option *o=opts;
		while (!o->sentinel) {
			//Find option, set boolean, and run a validator callback
			if ((o->sht && strcmp(*av, o->sht) == 0) || (o->lng && strcmp(*av, o->lng) == 0)) {
				o->set=1;
				if (o->callback) {
					if (!o->callback(++av, &o->v, buf))
						return 0;
				}
				else if (o->type == 'n' || o->type == 's' || o->type == 'c') {
					if (!opt_set_value(av, &o->v, o->type, buf)) {
						errprintf(buf);
						return 0;
					}
				}
			}
			o++;
		}
		av++;
	}
	return 1;
}
#endif



#ifdef LITE_RANDOM
#ifdef LITE_MT
#else
#endif

Random __rs;
static const struct { char *str; int len; } __rsdata[] = {
	{	"!\"#$%&'(),-./:;<=>?@[\\]^_`{|}~", 30 },
	{	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
		"ghijklmnopqrstuvwxyz"            , 52 },
	{	"0123456789"                      , 10 },
	{	"0123456789ABCDEFGHIJKLMNOPQRSTUV"
  	"WXYZabcdefghijklmnopqrstuvwxyz"  , 62 },
	{	"0123456789ABCDEFGHIJKLMNOPQRSTUV"
  	"WXYZabcdefghijklmnopqrstuvwxyz!#"
		"\"$%&'(),-./:;<=>?@[\\]^_`{|}~"  , 92 }
};


//Get a random seed
static int rand_seed_ctime (void)  {
 #ifdef LITE_MT
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	srand ( (unsigned) ts.tv_nsec );
 #else
	clock_gettime(CLOCK_REALTIME, &__rs.rand_ts);
	srand ( (unsigned) __rs.rand_ts.tv_nsec );
 #endif
	return 1;
}


//Feed to a stream
static char *rand_stream
#ifdef LITE_MT
 (Random *t, int type, int length)
#else
 (int type, int length) 
#endif
{
	memset(__rs.buf, 0, RAND_BUF_SIZE);
	rand_seed_ctime();
	int len = length < RAND_BUF_SIZE ? length : RAND_BUF_SIZE;
	for (int i=0; i<len; i++)
	{
	#ifdef LITE_MT
	#else
		__rs.buf[i] = __rsdata[type].str[( rand() % __rsdata[type].len )];
	#endif	
	}
	return __rs.buf;
}

//Return a random integer
int rand_number(void) {
	return (rand_seed_ctime()) ? rand() : 0;
}


//Return a string full of random punctuation 
char * rand_punct
#ifdef LITE_MT
 (Random *t, int length)
#else
 (int length) 
#endif
{
 #ifdef LITE_MT
	return NULL;
 #else
	return rand_stream(0, length);
 #endif
}


//Return a string full of random numbers
char * rand_numbers
#ifdef LITE_MT
 (Random *t, int length)
#else
 (int length) 
#endif
{
 #ifdef LITE_MT
	return NULL;
 #else
	return rand_stream(2, length);
 #endif
}


//Return a string full of random alphanumeric characters
char * rand_alnum
#ifdef LITE_MT
 (Random *t, int length)
#else
 (int length) 
#endif
{
 #ifdef LITE_MT
	return NULL;
 #else
	return rand_stream(3, length);
 #endif
}


//Return a string full of random alphabetical characters
char * rand_chars
#ifdef LITE_MT
 (Random *t, int length)
#else
 (int length) 
#endif
{
 #ifdef LITE_MT
	return NULL;
 #else
	return rand_stream(1, length);
 #endif
}

//Return a random string full of a = (ASCII characters), where 32 < a < 127
char * rand_any
#ifdef LITE_MT
 (Random *t, int length)
#else
 (int length) 
#endif
{
 #ifdef LITE_MT
	return NULL;
 #else
	return rand_stream(4, length);
 #endif
}

#ifdef LITE_EXPERIMENTAL
#if 0
//Return a random index from a set of pointers
void * rand_ptr
#ifdef LITE_MT
 (Random *t, void *ptr, int length)
#else
 (void *ptr, int length) 
#endif
{
	rand_seed_ctime();
 #ifdef LITE_MT
	return NULL;
 #else
	return ptr[ rand() % length ];
 #endif
}
#endif
#endif
#endif



#ifdef LITE_EXPERIMENTAL
#ifdef LITE_STRING
//Simple whitespace trimmer
char *string_trim (char *dest, const char *src, uint32_t len) {
	int start=0, end=0;

	for (int c=0; c <= len; c++)
		if ((start = (src[c] != 32) ? c : 0)) break;

	for (int c=len; c > 0; c--)
		if ((end   = (src[c] != 32 && src[c] != 9 && src != 0) ? c : 0)) break;

	//write(1, &src[start],  (end+1 - start));
	memcpy(dest, &src[start], ((end + 1) - start));
	dest[((end + 2) - start)] = '\0';
	return dest;
}
#endif


#ifdef LITE_HASH
static int modulos[] = {
	63,   127,  511, 1027, 2047,
	4095,8191,16383,32767,65535,
};


//this is a really bad implementation of DJB's hashing algo...  
static unsigned int lite_hash32 (const char *str, unsigned int len) {
	unsigned int hash=31;
	unsigned int s = strlen(str);	
	for (int i=0; i<len; i++) {
		hash += ((hash*31) + hash) + ((i > s) ? str[0] : str[i]);
	}
	return hash;
}


static unsigned int lite_uhash32 (const uint8_t *str, unsigned int slen, unsigned int len) {
	unsigned int hash=31;
	for (int i=0; i<len; i++) {
		hash += ((hash*31) + hash) + ((i > slen) ? str[0] : str[i]);
	}
	return hash;
}


//64 bit hash function (less collisions)
static unsigned long lite_hash64 (const char *str, unsigned int len) {
	unsigned long hash=5381;
	for (int i=0; i<len; i++) {	
		hash += ((hash << 5) + hash) + str[i];
	}
	return (hash / 2) + (hash % 2);
}


//Production builds do not have this enabled
#if 0 //#ifndef LITE_TEST
inline static 
#endif
unsigned int _hash (HashTable *h, const char *s) {
	return (lite_hash32(s, LITEU32_CMP_LEN) & LITEU32_MASK) % h->slots;
}	


#if 0 //#ifndef LITE_TEST
inline static 
#endif
unsigned int _uhash (HashTable *h, const char *s, int len) {
	return (lite_hash32(s, LITEU32_CMP_LEN) & LITEU32_MASK) % h->slots;
}	


//Save keys made from cstrings
void ht_save (HashTable *h, const char *key, void *value) {
	unsigned int hh   = _hash(h, key);  
	if ( !h->data[hh] )
		h->data[ hh ] = value;
	else {
		//Use open addressing to store these.	
		while ( h->data[hh] ) {
			h->check[ hh ] += 1;
			if ( !h->data[++hh] ) {
				h->data[ hh ] = value;
				break;
			}

			//If we run out of space,  the programmer ought to know
			if (hh == h->slots)
				return;
		}
	}
}


//Save keys using some other way of comparison
void ht_usave (HashTable *h, const uint8_t *key, int len, void *value) {
	//unsigned int hh   = (lite_uhash32(key, len, LITEU32_CMP_LEN) & LITEU32_MASK) % LITEU32_ARR_SZ;  
	//unsigned int hh   = (lite_uhash32(key, len, LITEU32_CMP_LEN) & LITEU32_MASK) % LITEU32_ARR_SZ;  
	//h->data[ hh ] = value;
}


//An example comparator function
static _Bool ht_cmp (const void *a, const void *b) {
	return ( strcmp( *(char **)a, *(char **)b ) == 0 );
}


//Get an item 
void *ht_get (HashTable *h, const char *key) {
	//Get the hash
	//unsigned int hh   = (lite_uhash32((uint8_t *)key, strlen(key), LITEU32_CMP_LEN) & LITEU32_MASK) % LITEU32_ARR_SZ;  
	unsigned int hh = _hash(h, key);  

	//We need to make sure that the key matches
	if ( !h->data[ hh ] )
		return NULL;
	//A custom comparator MAY not be needed depending on key
	else {
		//fprintf(stderr, "%s\n", key);
		if ( h->cmp( &key, &h->data[hh] ) )
			return h->data[hh];	
		else {
			fprintf(stderr, "keys didn't match: %s, %s\n", 
				key, (char *)h->data[hh]);
			for (int i=1; i <= h->check[hh]; i++) {
				if ( h->cmp( &key, &h->data[hh + i] ) )
					return h->data[hh + i];	
			}
			return NULL;
		}
	}
	return NULL;
}


#if 0 
//Initialize hash tables statically
HashTable *ht_stalloc (HashTable *h, int slots, _Bool (*cmp)(const void *, const void *)) 
{
	if (!h)
		return NULL;

	//Clear out or allocate memory
	memset(h, 0, sizeof(HashTable)); 

	//Error out when trying to create hash tables that are just way too big
	if (slots < 0 || slots > 65535)
		return NULL; //ERR_HASH_INVALID_SLOTS

	//Choose a sensible prime based on user's amount
	for (int n=0; n<sizeof(modulos)/sizeof(int); n++) {
		if ((slots * 5) < modulos[n]) {
			h->slots = modulos[n];
			break;
		}
	}

	//Allocate void ** and "tracker"
	h->data = malloc( sizeof(void *) * h->slots );
	h->check = malloc( h->slots );

	if (!h->data || !h->check)
		return NULL;  //ERR_HASH_MALLOC

	//Finally, set the comparator function (and fail if there isn't one)
	if ( !(h->cmp = cmp) ) 
		return NULL;  //ERR_NO_COMPARATOR

	//Set other stuff
	memset(h->data, 0, (sizeof(void *) * h->slots));
	memset(h->check, 0, h->slots);
	return h;
}
#endif



//Initialize (you can use void ** to do what you need to do, I think...)
HashTable *ht_alloc (HashTable *h, int slots, _Bool (*cmp)(const void *, const void *)) {
	//Clear out or allocate memory
	if (h)
		memset(h, 0, sizeof(HashTable)); 
	else {
		if ( !(h = calloc(sizeof(HashTable), 1)) ) {
			return NULL;
		}
	}

	//Error out when trying to create hash tables that are just way too big
	if (slots < 0 || slots > 65535)
		return NULL; //ERR_HASH_INVALID_SLOTS

	//Choose a sensible prime based on user's amount
	for (int n=0; n<sizeof(modulos)/sizeof(int); n++) {
		if ((slots * 5) < modulos[n]) {
			h->slots = modulos[n];
			break;
		}
	}

	//Allocate void ** and "tracker"
	h->data = malloc( sizeof(void *) * h->slots );
	h->check = malloc( h->slots );

	if (!h->data || !h->check)
		return NULL;  //ERR_HASH_MALLOC

	//Finally, set the comparator function (and fail if there isn't one)
	if ( !(h->cmp = cmp) ) 
		return NULL;  //ERR_NO_COMPARATOR

	//Set other stuff
	memset(h->data, 0, (sizeof(void *) * h->slots));
	memset(h->check, 0, h->slots);
	return h;
}



//Destroy
void ht_free (HashTable *h) {
	free(h->data);	
	free(h->check);	
	free(h);
}
#endif

#ifdef LITE_TOKENIZER
//static const plToken nulltok = { NULL, -1, -1, -1 }; 

#endif

#endif
