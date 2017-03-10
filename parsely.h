#ifndef PARSELY_H
#define PARSELY_H
/* -------------------------------------------- *
	parsely.h
	=========
	A general parsing library.  

	Usage
	=====
	1. Drop parsely.c and parsely.h into your project
	to start parsing some stuff fat free.

	<pre>
	cp parsely.[ch] /path/to/your/project
	</pre>


	2. Define a Parser structure somewhere within
	the function of importance (main() if this is
	just a quick and dirty program).

	<code>
	int main (int argc, char *argv[]) {
		Parser p;
	}
	</code>


	3. Assuming that you're looking to extract all
	blocks between '=' and '|', something like the
	following should do the job:

	<code>
	p.words = {
		{ "="  },
		{ "|"  },
		{ NULL },
	}
	</code>


	4. OPTIONAL: 
		Both of those steps can be combined for a short
		and sweet data structure declaration. (Assuming
		you compile your code with -std=c99 or something 
		similar.) 

	<code>
	int main (int argc, char *argv[]) {
		Parser p = {
			.words = {
				{ "="  },
				{ "|"  },
				{ NULL },
			}
		};
		return 0;
	}
	</code>


	5. Call pr_prepare by passing a reference to the
	Parser structure you just created.

	<code>
	int main (int argc, char *argv[]) {
		...
		pr_prepare( &p );
		return 0;
	}
	</code>


	6. Then use the pr_next iterator to pop results
	from whatever you are trying to loop through. So
	assuming you have static binary data and put the 
	contents in a variable titled 'src', read through 
	the file with pr_next by doing something similar 
	to the following:

	<code>
	int main (int argc, char *argv[]) {
		...
		unsigned char src[] = { 0x9f, 0x99, ... }; 	
		while ( pr_next( &p, src, sizeof(src) ) {
			write( 2, p.block, p.size );
		}
		return 0;
	}
	</code>


	7. Notice that p.block will point to the position 
	in your binary block where a matching string was
	found.  Alternatively, you can use p.prev to extract
	the blocks desired:

	<code>
	int main (int argc, char *argv[]) {
		...
		unsigned char src[] = { 0x9f, 0x99, ... }; 	
		while ( pr_next( &p, src, sizeof(src) ) {
			write( 2, &src[ p.prev ], p.size );
		}
		return 0;
	}
	</code>
	 

	Caveats:
	========
	Does not currently handle unsigned character
	data. This is because the library uses strlen to
	get the length of matches, negations and "catches".
	This will probably change in the future, at which
	point, this message will go away.

	Bugs:
	=====
	Occasionally, I find myself still futzing around 
	with start positions of some characters.  This will
	be fixed as well...
 
 * -------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#define PARSER_MAXMATCH_LEN 127
#define PARSER_MOD 31 
#define PARSER_MATCH_CATCH 1 << 1
#define PARSER_NEGATE_MATCH 1 << 0

typedef struct Parser Parser;
struct Parser {
	unsigned int   marker ;    //Keep track of bit flags
	unsigned char  founds
                 [255]  ;    //Each char goes here when building
	unsigned char  matchbuf    //buffer for matched tokens
                 [PARSER_MAXMATCH_LEN];     
	unsigned int   find;       //?
	unsigned char *match;      //ptr to matched token 
	unsigned char *word;       //ptr to found word
	unsigned int   tokenSize,  //size of found token
							   size,       //size of block between last match (or start) and current match
							   prev,       //previous match
							   next;       //next match
	struct words { 
		       char *word,       //Find this
                *catch,      //If you found *word, skip all other characters until you find this 
                *negate;     //If you found *word, skip *catch until you find this
	}              words[31];
};

void pr_prepare( Parser *p );
Parser *pr_next( Parser *p, unsigned char *src, int len );
#ifdef PARSELY_DEBUG
void pr_print ( Parser *p );
#endif
#endif
