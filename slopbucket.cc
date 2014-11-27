
// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details

#ifdef RCSID
static const char *const rcsid
= "@(#)$Id: slopbucket.cc,v 1.1.1.1 1998/01/19 15:46:47 jaw Exp $";
#endif


#include <conf.h>
#include <slopbucket.h>
#include <article.h>
#include <misc.h>

SlopBucket *backlog;

SlopBucket::SlopBucket(char *n){

	name = n;
	if( n ){
		slopp = fopen(name, "a");
	}else{
		slopp = 0;
	}
}

void SlopBucket::insert(Article *a){

	if( slopp ){
		fprintf(slopp, "%s %s %d %d\n",
			a->location, a->mid, a->size, a->timestamp);
	}
	DEBUG(2, "tossing %s into the slopbucket\n", a->mid);
	delete a;
}

