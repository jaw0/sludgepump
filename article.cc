

// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details

#ifdef RCSID
static const char *const rcsid
= "@(#)$Id: article.cc,v 1.1.1.1 1998/01/19 15:46:44 jaw Exp $";
#endif

#include <conf.h>
#include <article.h>
#include <misc.h>

#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>

extern "C" {
#include <configdata.h>
#include <raw.h>
}

extern int errno;
extern char *sys_errlist[];
extern timeval currenttime;

Article::Article(const char *m, const char *l, int s, long t){

	mid = strdup(m);
	location = strdup(l);

	size = s;
	timestamp = t;
	midhash = hash(mid);
	hnext = hprev = next = prev = 0;
	buf = 0;
	data = 0;
	mmapedp = 0;
}

Article::~Article(){

	free(mid);
	free(location);

	if( mmapedp ){
		munmap(mmapstart, mmaplen);
		mmapedp = 0;
		data = 0;
	}else{
		if( data )
			delete data;
	}
}

extern int RAWlastartsize;

// if you don't use CNFS or mmap
// futz around with this
int Article::get(){
	int fd, doff;
	RAWPART_OFF_T off;
	long dt;

	dt = NOW - timestamp;
	mmapedp = 0;
	
	if( USE_MMAP &&  dt < ROLLOVER_TIME ){
		off = RAWartnam2offset( location );
		fd  = RAWartopen( location, mid );
		if( fd == -1 )
			return 0;
		datasize = RAWlastartsize;

		doff = off - (off & ~8191);
		mmaplen = (datasize + doff + 8191 + sizeof(RAWARTHEADER)) & ~8191;
		
		mmapstart = mmap(0, mmaplen, PROT_READ, MAP_SHARED, fd, off & ~8191);
		if( mmapstart == (char*)-1 ){
			ERROR( sys_errlist[ errno ] );
			return 0;
		}
		mmapedp = 1;
		data = mmapstart + sizeof(RAWARTHEADER) + doff;
	}else{
		fd = RAWartopen( location, mid );
		datasize = RAWlastartsize;
		data = new char[datasize];
		read(fd, data, datasize);
	}
	return 1;
}
	
	
