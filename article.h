
// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details
// $Id: article.h,v 1.1.1.1 1998/01/19 15:46:44 jaw Exp $

#ifndef _article_h_
#define _article_h_

#include <conf.h>

class Buffer;

class Article {
public:

	Article *next,  *prev;
	Article *hnext, *hprev;
	Buffer *buf;


	char *mid;
	u_long midhash;
	char *location;
	int size;
	long timestamp;

	bool mmapedp;
#ifdef USE_MMAP
	char *mmapstart;
	int mmaplen;
#else
#endif	
	char *data;
	int datasize;

	Article(const char*mid, const char*loc, int size, long t);
	~Article();

	int get();		// open and read or mmap
	

};





#endif /* _article_h_ */
