
// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details
// $Id: nio.h,v 1.1.1.1 1998/01/19 15:46:44 jaw Exp $

#ifndef _nio_h_
#define _nio_h_

class NIO {
public:

	int fd;
	char *buffer;
	int bsize;

	char *start, *end;
	int llen;
	int err;
	int flags;
#define NIO_EOF		1	

	NIO(int fd, int size=0);
	char *getline();
	int fill();

};

#define NIO_SIZE 8192

#endif /* _nio_h_ */
