
// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details
// $Id: channel.h,v 1.1.1.1 1998/01/19 15:46:44 jaw Exp $

#ifndef _channel_h_
#define _channel_h_

#include <stats.h>
#include <buffer.h>

class Article;
class NIO;

class Channel {
public:
	int number;
	Channel *prev, *next;

	Buffer incoming, checked, outgoing, sent;
	Stats stats;
	NIO *io;

	bool streamingp;
	
	int maxsize;	// art size threshhold

	int (*statef)(Channel*, bool, bool, bool);
	void *privstate;		// private state info for statef
	
	bool wantr, wantw, wanta;
	long timeout;

	Channel();
	void insert(Article *);
	float reeval(float, int, float);
	void log();
	
};

extern Channel *chanlst;

int nntpopen(Channel*, bool, bool, bool);
int nntpclose(Channel*, bool, bool, bool);
int nntperror(Channel*, bool, bool, bool);

int nntpihave(Channel*, bool, bool, bool);
int nntpstream(Channel*, bool, bool, bool);


#endif /* _channel_h_ */

