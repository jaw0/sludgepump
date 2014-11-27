
// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details

#ifdef RCSID
static const char *const rcsid
= "@(#)$Id: nntpstream.cc,v 1.2 1998/01/19 18:36:10 jaw Exp $";
#endif


#include <conf.h>
#include <article.h>
#include <channel.h>
#include <misc.h>
#include <nio.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <stdio.h>
#include <sys/uio.h>

extern "C" {
#include <configdata.h>
#include <raw.h>
}

extern int errno;
extern const char *const sys_errlist[];
extern timeval currenttime;

struct StreamState {
	enum {
		Start,
		NeedArt,
		Work
	} s;
	int sent, datalen, writingp, needdotp, buflen;
	int checks_out, takes_out;
	int beenwarnedp;
	char *data;
	char buf[1024];
};

inline void setwants(Channel *c, StreamState *st){

	c->wanta = 1;
	c->wantr = c->wantw = 0;
	
	if( c->incoming.narts || c->outgoing.narts )
		c->wantw = 1;
	if( c->checked.narts || c->sent.narts )
		c->wantr = 1;
	if( st->writingp )
		c->wantw = 1;
	
	if( c->wantr || c->wantw ){
		st->s = StreamState::Work;
		c->timeout = NNTP_RESP_TIMEOUT + NOW;
	}else{
		// nothing to do
		c->timeout = 0;
		st->s = StreamState::NeedArt;
		c->stats.e_idlestart = NOW;
		c->stats.e_idleflag = 1;
	}
}

inline int readstuff(Channel *c, StreamState *st){
	char *line;
	Response r;
	Article *a;
	int tookp, ct;

	DEBUG(1, "StreamState::readstuff\n");
	
	while( !c->io->fill()){
		while( (line=c->io->getline()) ){
			DEBUG(4, "%d< %s\n", c->number, line);
			getresponse(line, &r);

			switch( r.code ){
			  case 400:
			  case 500:
			  case 480:
				if( r.msg )
					WARN( r.msg );
				else
					WARN( "chaos reigns" );
				return 0;
			  default:
				break;
			}

			ct = r.code / 10;
			ct %= 10;
			if( ct == 3 && (!r.msg || !r.msg[0] || (r.len < 4)) ){
				// an x3x response is s'posed to have a message id
				//   -- draft-ietf-nntpext-imp, section 1.3.1
				//
				// inn returns: "439\r\n"   - ick!
				// diablo: "439 <>\r\n"     - double ick!
				//
				if( !st->beenwarnedp ){
					WARN( "x3x response with no message-id, attempting to guess" );
					WARN( "you may wish to disable streaming for this site until "
					      "they fix their software" );
					st->beenwarnedp = 1;
				}
				
				// XXX - try to guess
				if( st->takes_out && c->sent.first ){
					// most likely a take response
					r.msg = c->sent.first->mid;
				}else if( st->checks_out && c->checked.first ){
					// most likely a check response
					r.msg = c->checked.first->mid;
				}else if( c->sent.first ){
					r.msg = c->sent.first->mid;
				}else if( c->checked.first ){
					r.msg = c->checked.first->mid;
				}else{
					ERROR( "unable to guess" );
					return 0;
				}
			}
			
			tookp = 0;
			a = c->checked.locate( r.msg );
			if( !a ){
				a = c->sent.locate( r.msg );
				tookp = 1;
			}
			if( !a ){
				ERROR("article disappeared");
				return 0;
			}

			if( tookp ){
				// this is a take response
				switch( r.code ){
				  case 239:
					c->stats.accepted++;
					break;
				  default:
					c->stats.refused ++;
					break;
				}
				c->sent.remove(a);
				st->takes_out --;
				delete a;
			}else{
				// this is a check response
				c->checked.remove(a);
				switch( r.code ){
				  case 238:
					c->outgoing.insert(a);
					break;
				  case 431:
					if( retrydefers ){
						c->insert(a);
						c->stats.deferred ++;
						break;
					}
					// fall thru
				  default:
					// not wanted
					delete a;
					c->stats.rejected ++;
				}
				st->checks_out --;
			}
			
		} // eo while getline
	} // eo while fill
	
	return 1;
}

inline int sendchecks(Channel *c, StreamState *st){
	int i, l, n;
	Article *a;

	DEBUG(1, "StreamState::sendchecks\n");

	st->buf[0] = 0;
	l = 0;
	for(i=0; i<CHECK_AT_A_TIME; i++){
		a = c->incoming.first;
		if(!a)
			break;
		if( !RAWartmaybehere(a->location) ){
			// art has been cancelled/over-written
			c->incoming.remove(a);
			delete a;
			continue;
		}
		// append "CHECK <mid>\r\n" to buffer
		for(n=0; n<6; n++)
			st->buf[l++] = "CHECK "[n];
		for(n=0; a->mid[n]; n++)
			st->buf[l++] = a->mid[n];
		st->buf[l++] = '\r';
		st->buf[l++] = '\n';
		st->buf[l]   = 0;

		DEBUG(1, "%d> CHECK %s\n", c->number, a->mid);
		c->incoming.remove(a);
		c->checked.insert(a);
		c->stats.offered ++;
		c->stats.e_offered ++;
		st->checks_out ++;
	}
	
	i = write(c->io->fd, st->buf, l);
	if( i < 0 ){
		ERROR( sys_errlist[ errno ] );
		return 0;
	}
	
	if( i != l ){
		// handle partial write
		st->sent = i;
		st->data = st->buf,
		st->datalen = l;
		st->writingp = 1;
		st->needdotp = 0;
	}
	return 1;
}

inline int sendtake(Channel *c, StreamState *st){
	Article *a;
	struct iovec iv[3];
	int i;
	
	DEBUG(1, "StreamState::sendtake\n");

	a = c->outgoing.first;
	if( !a ){
		ERROR("article disappeared");
		return 0;
	}
	if( !a->get()){
		// cancelled ?
		c->outgoing.remove(a);
		delete a;
		return 1;
	}

	sprintf(st->buf, "TAKETHIS %s\r\n", a->mid);
	st->buflen = strlen(st->buf);

	iv[0].iov_base = st->buf;
	iv[0].iov_len  = st->buflen;
	iv[1].iov_base = a->data;
	iv[1].iov_len  = a->datasize;
	iv[2].iov_base = ".\r\n";
	iv[2].iov_len  = 3;
	
	i = writev(c->io->fd, iv, 3);
	
	if( i < 0 ){
		ERROR( sys_errlist[ errno ] );
		return 0;
	}
	DEBUG(1, "%d> TAKETHIS %s\n", c->number, a->mid);
	DEBUG(4, "%d> [article data %d bytes of %d]\n", c->number, i, a->datasize + 13);
	
	// handle partial writes
	if( i < a->datasize + 13 ){
		st->sent = i;
		st->needdotp = 1;
		st->writingp = 1;
		st->data = a->data;
		st->datalen = a->datasize;
	}
	
	c->stats.bytessent += a->datasize;
	c->stats.e_bytessent += a->datasize;
	c->stats.e_maxsize = MAX(c->stats.e_maxsize, a->datasize);
	c->stats.e_minsize = c->stats.e_minsize ?
		MIN(c->stats.e_minsize, a->datasize) : a->datasize;
	
	c->outgoing.remove(a);
	c->sent.insert(a);
	st->takes_out ++;
	return 1;
}

int nntpstream(Channel *c, bool rp, bool wp, bool ap){
	StreamState *st;
	Article *a;
	char buffer[1024];
	
	if( !c->privstate ){
		// initial entry
		c->privstate = new StreamState;
		st = (StreamState*)c->privstate;
		st->s = StreamState::Start;
		st->checks_out = st->takes_out = 0;
		st->writingp = 0;
		st->beenwarnedp = 0;
	}
	st = (StreamState*)c->privstate;

	switch( st->s ){

	  case StreamState::Start:
	  again:
		DEBUG(5, "%d StreamState::Start\n", c->number);

		c->wantr = c->wantw = 0;
		c->wanta = 1;
		c->timeout = 0;
		st->s = StreamState::NeedArt;
		c->stats.e_idlestart = NOW;
		c->stats.e_idleflag = 1;
		if( !c->incoming.narts )
			break;
		// else fall thru
	  case StreamState::NeedArt:
		DEBUG(5, "%d StreamState::NeedArt %d %d %d\n", c->number, rp, wp, ap);
	
		if( ap || c->incoming.narts ){
			
			c->stats.e_idletime += NOW - c->stats.e_idlestart;
			c->stats.e_idleflag = 0;
			c->stats.e_times ++;
			c->wanta = c->wantr = 0;
			c->wantw = 1;
			c->timeout = NNTP_SEND_TIMEOUT + NOW;
			st->writingp = 0;
			st->s = StreamState::Work;
		}else{
			// close down the connection
			delete st;
			c->privstate = 0;
			c->wantr = c->wantw = c->wanta = 0;
			c->timeout = 0;
			c->statef = nntpclose;
		}
		break;

	  case StreamState::Work:
		DEBUG(5, "%d StreamState::Work %d %d %d\n", c->number, rp, wp, ap);
	
		if( rp ){
			// read stuff
			if( !readstuff(c, st))
				goto error;
		}
		if( wp ){
			// write stuff
			if( st->writingp ){
				// finish a write in progress
				struct iovec iv[3];
				int i;

				if( st->needdotp ){

					if( st->sent > st->datalen + st->buflen){
						int x;

						x = st->sent - st->datalen - st->buflen;
						i = write(c->io->fd, ".\r\n" + x, 3-x);
						
					}else if( st->sent > st->buflen ){
						iv[0].iov_base = st->data + st->sent - st->buflen;
						iv[0].iov_len  = st->datalen - st->sent + st->buflen;
						iv[1].iov_base = ".\r\n";
						iv[1].iov_len  = 3;
						
						i = writev(c->io->fd, iv, 2);
					}else{
						iv[0].iov_base = st->buf + st->sent;
						iv[0].iov_len  = st->buflen - st->sent;
						iv[1].iov_base = a->data;
						iv[1].iov_len  = a->datasize;
						iv[2].iov_base = ".\r\n";
						iv[2].iov_len  = 3;

						i = writev(c->io->fd, iv, 3);
					}
					
				}else{
					i = write(c->io->fd, st->data + st->sent, st->datalen - st->sent);
				}

				if( i < 0 ){
					ERROR( sys_errlist[ errno ] );
					goto error;
				}
				st->sent += i;
				if( st->sent >= st->datalen + st->needdotp?13:0 ){
					st->writingp = 0;
					st->needdotp = 0;
				}
			}else if( c->outgoing.narts ){
				// send TAKETHISs
				if( !sendtake(c, st))
					goto error;
			}else{
				// send checks
				if( !sendchecks(c, st))
					goto error;
			}
		}

		if( rp || wp || ap ){
			setwants(c, st);
			break;
		}
		WARN("timed out");
		goto error;
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

