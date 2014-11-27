
// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details
// $Id: slopbucket.h,v 1.1.1.1 1998/01/19 15:46:44 jaw Exp $

#ifndef _slopbucket_h_
#define _slopbucket_h_

struct __sFILE;
typedef struct __sFILE FILE;
class Article;

// backlogs get heaved into the SlopBucket

class SlopBucket {
public:
	FILE *slopp;
	char *name;

	void insert(Article *);

	SlopBucket(char *);

};

extern SlopBucket *backlog;

#endif /* _slop_bucket_h_ */

