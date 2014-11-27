

// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details

#ifdef RCSID
static const char *const rcsid
= "@(#)$Id: nntp.cc,v 1.1.1.1 1998/01/19 15:46:45 jaw Exp $";
#endif


#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/time.h>

#include <conf.h>
#include <channel.h>
#include <article.h>
#include <misc.h>
#include <nio.h>

extern int errno;
extern const char *const sys_errlist[];
extern int donotstream;
extern timeval currenttime;

struct OpenState {
	enum {
		Start,
		Connected,
		GetGreeting,
		SendMode,
		GetMode
	} s;
};

int nntpopen(Channel *c, bool rp, bool wp, bool ap){
	OpenState *st;
	Article *a;
	int fd, i;

	if( !c->privstate ){
		// initial entry
		c->privstate = new OpenState;
		st = (OpenState*)c->privstate;
		st->s = OpenState::Start;
	}
	st = (OpenState*)c->privstate;


	switch( st->s ){

	  case OpenState::Start:
		DEBUG(5, "%d OpenState::Start\n", c->number);
		struct sockaddr_in sa;

		// move checked to incoming
		while( (a=c->checked.first)){
			c->checked.remove(a);
			c->incoming.insert(a);
		}

		// delete sent
		while( (a=c->sent.first)){
			c->sent.remove(a);
			delete a;
		}
		
		fd = socket(AF_INET, SOCK_STREAM, 0);
		SetNonBlocking(fd, 1);

#if 0
		// ???
		i = 64 * 1024;
		setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &i, sizeof(i));
		setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &i, sizeof(i));
#endif
		
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = ntohl(hostip);
		// why is htonl a nop on NetBSD?
		sa.sin_port = htons(hostport);
		
		i = connect(fd, (struct sockaddr*)&sa, sizeof(sa));
		if( i< 0 && errno != EINPROGRESS){
			close(fd);
			ERROR("connect failed");
			goto error;
		}

		c->io = new NIO(fd);
		c->stats.connects ++;
		
		c->wantw = 1;
		c->wantr = c->wanta = 0;
		c->timeout = NNTP_OPEN_TIMEOUT + NOW;
		st->s = OpenState::Connected;

		break;

	  case OpenState::Connected:
		DEBUG(5, "%d OpenState::Connected %d %d %d\n", c->number, rp, wp, ap);
		
		if( wp ){
			c->wantr = 1;
			c->wantw = c->wanta = 0;
			c->timeout = NNTP_RESP_TIMEOUT + NOW;
			st->s = OpenState::GetGreeting;
		}else{
			WARN("connect failed");
			goto error;
		}
		break;

	  case OpenState::GetGreeting:
		DEBUG(5, "%d OpenState::GetGreeting %d %d %d\n", c->number, rp, wp, ap);
		
		if( rp ){
			char *line;
			Response r;

			if( c->io->fill() ){
				WARN( sys_errlist[ c->io->err ] );
				goto error;
			}
			line = c->io->getline();
			DEBUG(4, "%d< %s\n", c->number, line);
			getresponse(line, &r);

			if( r.code != 200 ){
				ERROR(r.msg);
				goto error;
			}else{
				c->wantw = 1;
				c->wantr = c->wanta = 0;
				c->timeout = NNTP_SEND_TIMEOUT + NOW;
				st->s = OpenState::SendMode;
			}
		}else{
			WARN("timed out waiting for greeting");
			goto error;
		}
		break;

	  case OpenState::SendMode:
		DEBUG(5, "%d OpenState::SendMode %d %d %d\n", c->number, rp, wp, ap);
		
		if( wp ){
			DEBUG(4, "%d> mode stream\n", c->number);
			write(c->io->fd, "mode stream\r\n", 13);
			c->wantr = 1;
			c->wantw = c->wanta = 0;
			c->timeout = NNTP_RESP_TIMEOUT + NOW;
			st->s = OpenState::GetMode;
		}else{
			WARN("timed out waiting to send");
			goto error;
		}
		break;

	  case OpenState::GetMode:
		DEBUG(5, "%d OpenState::GetMode  %d %d %d\n", c->number, rp, wp, ap);
		
		if( rp ){
			char *line;
			Response r;

			if( c->io->fill() ){
				WARN( sys_errlist[ c->io->err ] );
				goto error;
			}
			line = c->io->getline();
			DEBUG(4, "%d< %s\n", c->number, line);
			getresponse(line, &r);

			if( r.code == 203 ){
				c->streamingp = 1; 
			}else{
				c->streamingp = 0;
			}

			delete st;
			c->privstate = 0;
			c->timeout = 0;
			c->wantr = c->wantw = c->wanta = 0;

			if( c->streamingp && !donotstream )
				c->statef = nntpstream;
			else
				c->statef = nntpihave;
			
		}else{
			WARN("timed out waiting for response");
			goto error;
		}
		break;
	}

	return 1;

  error:
	delete st;
	c->privstate = 0;
	c->timeout = 0;
	c->wantr = c->wantw = c->wanta = 0;
	c->statef = nntperror;
	return 0;
}


struct CloseState {
	enum {
		Start,
		SendQuit,
		GetQuit
	} s;
};

int nntpclose(Channel *c, bool rp, bool wp, bool ap){
	CloseState *st;
	int fd, i;

	
	if( !c->privstate ){
		// initial entry
		c->privstate = new CloseState;
		st = (CloseState*)c->privstate;
		st->s = CloseState::Start;
	}
	st = (CloseState*)c->privstate;

	switch( st->s ){
		
	  case CloseState::Start:
		DEBUG(5, "%d CloseState::Start\n", c->number);
		c->wantw = 1;
		c->wantr = c->wanta = 0;
		c->timeout = NNTP_SEND_TIMEOUT + NOW;
		st->s = CloseState::SendQuit;
		break;

	  case CloseState::SendQuit:
		DEBUG(5, "%d CloseState::SendQuit %d %d %d\n", c->number, rp, wp, ap);
		
		if( wp ){

			DEBUG(4, "%d> QUIT\n", c->number);
			write(c->io->fd, "QUIT\r\n", 6);
			c->wantr = 1;
			c->wantw = c->wanta = 0;
			c->timeout = NNTP_RESP_TIMEOUT + NOW;
			st->s = CloseState::GetQuit;
		}else{
			WARN("timed out while waiting to send");
			goto error;
		}
		break;

	  case CloseState::GetQuit:
		DEBUG(5, "%d CloseState::GetQuit %d %d %d\n", c->number, rp, wp, ap);
		
		if( rp ){
			if( c->io->fill() ){
				WARN( sys_errlist[ c->io->err ] );
				goto error;
			}
			// do I need to parse the response?
			close(c->io->fd);
			delete c->io;
			c->io = 0;

			c->wanta = c->wantw = c->wantr = 0;
			delete st;
			c->privstate = 0;
			// ??? where to?
			c->statef = 0;
		}else{
			goto error;
		}
		break;

	}
	return 1;

  error:
	delete st;
	c->privstate = 0;
	c->timeout = 0;
	c->wantr = c->wantw = c->wanta = 0;
	c->statef = nntperror;
	return 0;
}

struct ErrorState {
	enum {
		Start,
		TimedOut
	} s;
};

int nntperror(Channel *c, bool rp, bool wp, bool ap){
	ErrorState *st;
	int fd, i;

	if( !c->privstate ){
		// initial entry
		c->privstate = new ErrorState;
		st = (ErrorState*)c->privstate;
		st->s = ErrorState::Start;
	}
	st = (ErrorState*)c->privstate;

	switch( st->s ){
		
	  case ErrorState::Start:
		DEBUG(5, "%d ErrorState::Start\n", c->number);
		
		if( c->io ){
			// slam it shut
			close(c->io->fd);
			delete c->io;
			c->io = 0;
		}
		c->wantw = c->wantr = c->wanta = 0;
		// force channel to pause
		c->timeout = ERROR_REOPEN_TIME + NOW;
		st->s = ErrorState::TimedOut;
		break;

	  case ErrorState::TimedOut:
		DEBUG(5, "%d ErrorState::TimedOut\n", c->number);
		
		c->wantw = c->wantr = c->wanta = 0;
		c->timeout = 0;
		delete st;
		c->privstate = 0;
		if( inn->flags & NIO_EOF){
			c->statef = 0;
		}else{
			c->statef = nntpopen;	// re-open
		}
		break;
	}

	return 1;
}


		
		
