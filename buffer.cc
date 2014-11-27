

// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details

#ifdef RCSID
static const char *const rcsid
= "@(#)$Id: buffer.cc,v 1.1.1.1 1998/01/19 15:46:45 jaw Exp $";
#endif


#include <buffer.h>
#include <article.h>
#include <channel.h>
#include <misc.h>

#include <string.h>


Buffer::Buffer(){
	int i;
	
	narts = 0;
	first = last = 0;
	chan = 0;

	for(i=0; i<HASHTABLESIZE; i++)
		htable[i] = 0;

}

// implement FIFO semantics
void Buffer::insert(Article *art){
	int h;

	ASSERT( !art->buf );
	h = art->midhash % HASHTABLESIZE;
	
	// add to fifo
	art->next = 0;
	art->prev = last;
	art->buf = this;
	if( last )
		last->next = art;
	last = art;

	if( ! first )
		first = art;

	// add to hash table
	art->hnext = htable[h];
	art->hprev = 0;
	if( htable[h] )
		htable[h]->hprev = art;
	htable[h] = art;

	narts ++;
	
	ASSERT( narts && first || !narts && !first );
}

void Buffer::remove(Article *art){

	ASSERT( this == art->buf );
	
	// remove from fifo
	if( art->prev )
		art->prev->next = art->next;
	else
		first = art->next;
	
	if( art->next )
		art->next->prev = art->prev;
	else
		last = art->prev;
	
	art->next = art->prev = 0;

	// remove from hash table
	if( art->hnext )
		art->hnext->hprev = art->hprev;

	if( art->hprev )
		art->hprev->hnext = art->hnext;
	else
		htable[art->midhash % HASHTABLESIZE] = art->hnext;

	art->hnext = art->hprev = 0;
	art->buf = 0;

	narts --;

	ASSERT( narts && first || !narts && !first );
}

Article *Buffer::locate(const char *mid, int h){
	Article *a;

	a = htable[ h % HASHTABLESIZE ];

	while( a ){
		if( !strcmp(mid, a->mid))
			return a;

		a = a->hnext;
	}
	return 0;
	
}

	
