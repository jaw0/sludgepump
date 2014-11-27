
// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details
// $Id: misc.h,v 1.1.1.1 1998/01/19 15:46:45 jaw Exp $


#ifndef _misc_h_
#define _misc_h_

#include <conf.h>
#include "/dev/null"	// for bug reports and user complaints

class NIO;
class SlopBucket;

class Response {
public:
	int code;
	int len;
	char *msg;
};


// some globals
extern char *hostname;
extern u_long hostip;
extern u_short hostport;

extern NIO *inn;		// feed from innd
extern SlopBucket *backlog;	// backlogs

extern int maxchans;		// maximum open channels
extern int currchans;		// current open channels
extern int debuglevel;
extern int retrydefers;
extern int logevery;

extern int hash(const char*);
extern u_long get_ipaddr(const char *);
extern void error(int, const char*, const char*, int);
extern void getresponse(char *, Response*);
extern void debug(int, const char*, ...);

#define WARN(m)		error(0, m, __FILE__, __LINE__)
#define ERROR(m)	error(1, m, __FILE__, __LINE__)
#define FATAL(m)	error(2, m, __FILE__, __LINE__)

#define ASSERT(ex)	{ if (!(ex)){ FATAL("Assertion failed"); } }

#define DEBUG		if( debuglevel ) debug

#define NOW		currenttime.tv_sec

#ifndef MIN
#  define MIN(a,b)	((a)<(b)?(a):(b))
#endif
#ifndef MAX
#  define MAX(a, b)	((a)>(b)?(a):(b))
#endif

#endif /* _misc_h_ */


	  
