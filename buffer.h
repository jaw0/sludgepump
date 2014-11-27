
// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details
// $Id: buffer.h,v 1.1.1.1 1998/01/19 15:46:44 jaw Exp $

#ifndef _buffer_h_
#define _buffer_h_

#define HASHTABLESIZE	1021

class Article;
class Channel;

extern int hash(const char*);
class Buffer {
public:

	Channel *chan;		// back ptr

	Article *first, *last;	// list
	Article *htable[1021];	// hash table
	
	int narts;		// number of articles
	
	
	Buffer();
	void insert(Article*);
	void remove(Article*);
	Article *locate(const char*mid)		{return locate(mid, hash(mid));}
	Article *locate(const char*, int);	// locate article in buffer by m-id

};




#endif /* ! _buffer_h_ */
