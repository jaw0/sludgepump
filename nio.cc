

// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details

#ifdef RCSID
static const char *const rcsid
= "@(#)$Id: nio.cc,v 1.1.1.1 1998/01/19 15:46:44 jaw Exp $";
#endif


#include <conf.h>
#include <nio.h>

extern int errno;

NIO::NIO(int ifd, int size){

	fd = ifd;

	if(!size)
		size = NIO_SIZE;
	bsize = 2 * size;		// double buffer
	buffer = new char[bsize];
	start = end = buffer + bsize/2;
	llen = 0;
	errno = 0;
	flags = 0;
}

int NIO::fill(){
	char *p;
	int n;
	
	// move any existing data to bottom
	p = buffer + bsize / 2;
	while( start < end ){
		*--p = *--end;
	}
	start = p;
	end = buffer + bsize / 2;

	// suck on the hose
	n = read(fd, buffer + bsize/2, bsize/2);

	if(n<0){
		err = errno;
	}else if(n==0){
		// eof
		err = -1;
		flags |= NIO_EOF;
	}else{
		err = 0;
		end += n;
	}
	return err;
}

char *NIO::getline(){
	char *p, *x;

	if( end <= start )
		return 0;

	p = memchr(start, '\n', end - start);
	if(p){
		*p = 0;
		llen = p - start;
		x = start;
		start = p+1;
		if( *(p-1) == '\r' ){
			// strip it off
			*(p-1) = 0;
			llen --;
		}
		return x;
	}
	return 0;
}

		
	
