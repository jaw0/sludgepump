

// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details

#ifdef RCSID
static const char *const rcsid
= "@(#)$Id: nntpihave.cc,v 1.1.1.1 1998/01/19 15:46:45 jaw Exp $";
#endif


#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <stdio.h>
#include <sys/uio.h>
#include <sys/time.h>

#include <conf.h>
#include <article.h>
#include <channel.h>
#include <misc.h>
#include <nio.h>

extern "C" {
#include <configdata.h>
#include <raw.h>
}

extern int errno;
extern const char *const sys_errlist[];
extern FILE *dbgfp;
extern timeval currenttime;

struct IhaveState {
	enum {
		Start,
		NeedArt,
		SendIhave,
		GetIhaveResp,
		SendArt,
		SendingArt,
		GetSentResp
	} s;
	int sent;
};

int nntpihave(Channel *c, bool rp, bool wp, bool ap){
	IhaveState *st;
	Article *a;
	char buffer[1024];
	
	if( !c->privstate ){
		// initial entry
		c->privstate = new IhaveState;
		st = (IhaveState*)c->privstate;
		st->s = IhaveState::Start;
	}
	st = (IhaveState*)c->privstate;

	switch( st->s ){

	  case IhaveState::Start:
	  again:
		DEBUG(5, "%d IhaveState::Start\n", c->number);

		c->wantr = c->wantw = 0;
		c->wanta = 1;
		c->timeout = 0;
		st->s = IhaveState::NeedArt;
		c->stats.e_idlestart = NOW;
		c->stats.e_idleflag = 1;
		if( ! c->incoming.narts )
			break;
		// else fall thru
	  case IhaveState::NeedArt:
		DEBUG(5, "%d IhaveState::NeedArt %d %d %d\n", c->number, rp, wp, ap);
	
		if( ap || c->incoming.narts ){
			
			c->stats.e_idletime += NOW - c->stats.e_idlestart;
			c->stats.e_idleflag = 0;
			c->stats.e_times ++;
			c->wanta = c->wantr = 0;
			c->wantw = 1;
			c->timeout = NNTP_SEND_TIMEOUT + NOW;
			st->s = IhaveState::SendIhave;
		}else{
			// close down the connection
			delete st;
			c->privstate = 0;
			c->wantr = c->wantw = c->wanta = 0;
			c->timeout = 0;
			c->statef = nntpclose;
		}
		break;

	  case IhaveState::SendIhave:
		DEBUG(5, "%d IhaveState::SendIhave %d %d %d\n", c->number, rp, wp, ap);
	
		if( wp ){
			int i, l;
			a = c->incoming.first;
			if(!a){
				// back a state - another channel must have stolen article
				goto again;
			}
			if( !RAWartmaybehere(a->location) ){
				// art has been cancelled/over-written
				c->incoming.remove(a);
				delete a;
				goto again;
			}

			sprintf(buffer, "IHAVE %s\r\n", a->mid);
			l = strlen(buffer);
			DEBUG(4, "%d> IHAVE %s\n", c->number, a->mid);
			i = write(c->io->fd, buffer, l);
			if( i != l ){
				ERROR("send IHAVE failed");
				goto error;
			}

			c->stats.offered ++;
			c->stats.e_offered ++;
			
			c->incoming.remove(a);
			c->checked.insert(a);
			
			c->wantw = c->wanta = 0;
			c->wantr = 1;
			c->timeout = NNTP_RESP_TIMEOUT + NOW;
			st->s = IhaveState::GetIhaveResp;
		}else{
			WARN("timed out while waiting to send");
			goto error;
		}
		break;

	  case IhaveState::GetIhaveResp:
		DEBUG(5, "%d IhaveState::GetIhaveResp %d %d %d\n", c->number, rp, wp, ap);

		if( rp ){
			char *line;
			Response r;

			if( c->io->fill() ){
				if( c->io->flags && NIO_EOF ){
					WARN( "remote site closed the connection" );
					goto error;
				}
				WARN( sys_errlist[ c->io->err ] );
				// retry
				break;
			}
			line = c->io->getline();
			DEBUG(4, "%d< %s\n", c->number, line);
			getresponse(line, &r);

			a = c->checked.first;
			if( !a ){
				ERROR("article disappeared");
				goto error;
			}
			switch( r.code ){
			  case 335:
				// send it
				c->checked.remove(a);
				c->outgoing.insert(a);
				c->wantw = 1;
				c->wanta = c->wantr = 0;
				c->timeout = NNTP_SEND_TIMEOUT + NOW;
				st->s = IhaveState::SendArt;
				break;

			  default:
				// not wanted
				c->checked.remove(a);
				delete a;
				c->stats.rejected ++;

				// if we already have an art, just do it...
				goto again;
				break;
			}
		}else{
			WARN("timed out while waiting for response");
			goto error;
		}
		break;
		
	  case IhaveState::SendArt:
		DEBUG(5, "%d IhaveState::SendArt %d %d %d\n", c->number, rp, wp, ap);

		if( wp ){
			struct iovec iv[2];
			int i;
			
			a = c->outgoing.first;
			if( !a ){
				ERROR("article disappeared");
				goto error;
			}
			if( !a->get()){
				// cancelled ?
				c->outgoing.remove(a);
				delete a;
				goto again;
			}

			iv[0].iov_base = a->data;
			iv[0].iov_len  = a->datasize;
			iv[1].iov_base = ".\r\n";
			iv[1].iov_len  = 3;

			i = writev(c->io->fd, iv, 2);
			
			if( i < 0 ){
				ERROR( sys_errlist[ errno ] );
				if( errno == EFAULT ){
					// XXX - I do not know why this happens
					DEBUG(1, "efault: data=%x size=%d\n", a->data, a->datasize);
				}
				goto error;
			}
			DEBUG(4, "%d> [article data %d bytes of %d]\n", c->number, i, a->datasize + 3);

			// handle partial writes
			if( i < a->datasize + 3 ){
				st->sent = i;
				c->wantw = 1;
				c->wanta = c->wantr = 0;
				c->timeout = NNTP_SEND_TIMEOUT + NOW;
				st->s = IhaveState::SendingArt;
			}else{
				c->wantw = c->wanta = 0;
				c->wantr = 1;
				c->timeout = NNTP_RESP_TIMEOUT + NOW;
				st->s = IhaveState::GetSentResp;
			}

			c->outgoing.remove(a);
			c->sent.insert(a);
		}else{
			WARN("timed out while waiting to send");
			goto error;
		}
		break;

	  case IhaveState::SendingArt:
		DEBUG(5, "%d IhaveState::SendingArt %d %d %d\n", c->number, rp, wp, ap);

		if( wp ){
			struct iovec iv[2];
			int i;
			
			a = c->sent.first;
			if( !a ){
				ERROR("article disappeared");
				goto error;
			}

			if( st->sent < a->datasize ){
				iv[0].iov_base = a->data + st->sent;
				iv[0].iov_len  = a->datasize - st->sent;
				iv[1].iov_base = ".\r\n";
				iv[1].iov_len  = 3;
				
				i = writev(c->io->fd, iv, 2);
			}else{
				int x;

				x = st->sent - a->datasize;
				i = write(c->io->fd, ".\r\n" + x, 3 - x);
			}

			if( i < 0 ){
				ERROR( sys_errlist[ errno ] );
				goto error;
			}
			DEBUG(4, "%d> [more article data %d bytes]\n", c->number, i);

			st->sent += i;

			if( st->sent >= a->datasize + 3 ){
				c->wantw = c->wanta = 0;
				c->wantr = 1;
				c->timeout = NNTP_RESP_TIMEOUT + NOW;
				st->s = IhaveState::GetSentResp;
			}else{
				// stay here until all sent
				c->timeout = NNTP_SEND_TIMEOUT + NOW;
			}
		}else{
			WARN("timed out while sending");
			goto error;
		}
		break;

	  case IhaveState::GetSentResp:
		DEBUG(5, "%d IhaveState::GetSentResp %d %d %d\n", c->number, rp, wp, ap);

		if( rp ){
			char *line;
			Response r;

			a = c->sent.first;
			if( !a ){
				ERROR("article disappeared");
				goto error;
			}

			if( c->io->fill() ){
				WARN( sys_errlist[ c->io->err ] );
				// start over
				break;
			}
			line = c->io->getline();
			DEBUG(4, "%d< %s\n", c->number, line);
			getresponse(line, &r);

			switch( r.code ){

			  case 235:
				// all done
				c->sent.remove(a);
				// keep stats
				c->stats.accepted ++;
				c->stats.e_accepted ++;
				c->stats.bytessent += a->datasize;
				c->stats.e_bytessent += a->datasize;
				c->stats.e_maxsize = MAX(c->stats.e_maxsize, a->datasize);
				c->stats.e_minsize = c->stats.e_minsize ?
					MIN(c->stats.e_minsize, a->datasize) : a->datasize;
				
				delete a;
				break;

			  case 436:
				if( retrydefers ){
					// try again later
					c->sent.remove(a);
					c->insert(a);
					c->stats.deferred ++;
					break;
				}
				// fall thru
			  default:
				// article not wanted
				c->sent.remove(a);
				delete a;
				c->stats.refused ++;
				break;
			}
			goto again;

		}else{
			WARN("timed out while waiting for response");
			goto error;
		}
		break;
	}
	return 1;

  error:
	delete st;
	c->privstate = 0;
	c->wantr = c->wantw = c->wanta = 0;
	c->timeout = 0;
	c->statef = nntperror;
	return 0;
}

