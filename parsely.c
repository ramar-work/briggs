#include "parsely.h"

//prepare
void pr_prepare( Parser *p )
{
#if 0
	//
	if (len > INT_MAX) {
		return NULL;
	}
#endif

	//Clear found character table and set hash table to all -1	
	memset(p->founds, 0, sizeof(p->founds));
			
	//Extract first character and use it as a truth index
	for (int i=0; i<32; i++) {
		struct words *w = &p->words[i];
		if ( !w->word ) 
			return;
		p->founds[ (int)w->word[0] ] = 1;
	}
}


//We might want the match, or readahead (this can be specified by the character)
Parser *pr_next( Parser *p, unsigned char *src, int len ) 
{
	//p->prev is always 0 at first
	p->prev  = p->next;
	p->match = (unsigned char *)p->matchbuf;

	//Set current pointer to current position
	p->word  = &src[p->prev];

	//loop through the entire block
	while ( p->next < len ) {
		/*Stop skipping if this is a match*/
		if ( (p->marker & PARSER_MATCH_CATCH) && *p->word == *p->words[p->find].catch ) 
		{
			int nl = len - p->next;
			int wl = 0;
			char *m = p->words[p->find].catch;

#if 0
			//Check if it's supposed to be negated
			if ( p->words[p->find].negate ) {
			} 
			if ( (p->marker & PARSER_NEGATE_MATCH) ) {
			} 
#endif
			
			//Check that this is indeed a match
			if ( (nl > (wl = strlen(m))) && memcmp(p->word, m, wl) == 0 ) {
				//p->prev += 1/*why the extra 1?*/;
				memset(p->match, 0, PARSER_MAXMATCH_LEN);
				memcpy(p->match, &src[p->prev], p->tokenSize);
				p->prev += p->tokenSize/*why the extra 1?*/;
				p->size = (p->next /*+= p->tokenSize*/ /*+ wl*/) - p->prev;
				p->next += p->tokenSize;
				p->marker = 0x00;
				return p;	
			}
		}

		/*Found a single character from rng*/
		if ( !p->marker && p->founds[ (int)*p->word ] )  
		{
			int nl = len - p->next;
			int wl = 0;
			p->find = -1;

			//Check that this is a real token match 
			for (int i=0; i<32; i++) {
				struct words *w = &p->words[i];
				if ( !w->word )
					break;
				//If there's a match, mark things accordingly
				if ( (nl > (wl = strlen(w->word))) && memcmp( p->word, w->word, wl ) == 0 ) {
					p->find = i;
					p->tokenSize = wl;
					break;
				}
			}

			//write the matching token here
			if ( p->find == -1 ) 
				p->tokenSize = 0;
			//check if it's a "bit" token
			else if ( p->words[p->find].catch )
				p->marker |= PARSER_MATCH_CATCH;
			//or just handle it
			else {
				//Copy token to match pointer
				memset(p->match, 0, PARSER_MAXMATCH_LEN);
				memcpy(p->match, p->word, p->tokenSize);
				p->match[p->tokenSize + 1] = '\0';

				//set the block size
				p->size  = (p->next /*+= p->tokenSize*/) - p->prev;
				p->next += p->tokenSize;
				return p;
			}
		}

		//move up through both characters
		p->word++, p->next++;
	}

	//
	return NULL;
}


#ifdef PARSELY_DEBUG
//print a parser structure
void pr_print ( Parser *p ) {
	fprintf ( stderr, "p->marker   %d\n", p->marker );
	fprintf ( stderr, "p->size     %d\n", p->size   );
	fprintf ( stderr, "p->prev     %d\n", p->prev   );
	fprintf ( stderr, "p->next     %d\n", p->next   );
	fprintf ( stderr, "p->word     %s\n", p->word );
	fprintf ( stderr, "p->match    %s\n", p->match );
}
#endif
