

// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details

#ifdef RCSID
static const char *const rcsid
= "@(#)$Id: misc.cc,v 1.1.1.1 1998/01/19 15:46:45 jaw Exp $";
#endif


#include <conf.h>
#include <misc.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>

extern FILE *dbgfp;

u_long get_ipaddr(const char *host){
	struct hostent *hp;

	if((hp = gethostbyname(host)) == 0) {
		return 0;
	}
	return ntohl(*(u_long *)hp->h_addr);
}

int hash(const char *p){
	int hv = 0;

	while(*p)
		hv = hv<<1 ^ *p++;
        hv = hv<0 ? -hv : hv;
        return hv;
}

void getresponse(char *l, Response *r){
	char *p;

	r->msg = 0;
	r->len = 0;

	p = strchr(l, ' ');
	if(!p) p = strchr(l, '\t');	// be lenient in what you accept
	
	if( p ){
		*p = 0;
		r->msg = p + 1;
		r->len = strlen(r->msg);
	}

	r->code = atoi( l );
}


void error(int n, const char *msg, const char *file, int line){
	static int nerrs = 0;
	char buf[1024];
	char *b;

	switch(n){
	  case 0: b = "WARNING: "; break;
	  case 1: b = "ERROR: "; break;
	  case 2: b = "FATAL: "; break;
	  default: b = ""; break;
	}
	sprintf(buf, "%s%s %s (in %s line %d)\n", b, hostname, msg, file, line);

	DEBUG(1, buf);
	syslog(LOG_ERR, buf);

	if( n == 2 )
		abort();
	if( n == 1 && nerrs ++ > QUIT_AFTER_ERRORS )
		exit(-1);
}

void debug(int level, const char *m, ...){
	va_list ap;
	char buf[1024];
	
        va_start(ap, m);

	if( level <= debuglevel && dbgfp ){
		sprintf(buf, "%s: %s", hostname, m);
		vfprintf(dbgfp, buf, ap);
		fflush(dbgfp);
	}
}


