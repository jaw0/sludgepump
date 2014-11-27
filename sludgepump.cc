
// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details

#ifdef RCSID
static const char *const rcsid
= "@(#)$Id: sludgepump.cc,v 1.2 1998/01/19 18:36:11 jaw Exp $";
#endif


#include <conf.h>
#include <channel.h>
#include <article.h>
#include <nio.h>
#include <stats.h>
#include <misc.h>

#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <sys/errno.h>
#include <math.h>


extern int errno;
extern const char *const sys_errlist[];

struct timeval currenttime;

inline void newarticle(char *line){
	Article *a;
	char *m, *l, *p, *pp;
	int s, t;
	
	// location mid size timestamp
	l = line;
	p = strchr(l, ' ');
	if(!p){
		ERROR("corrupt input: use Wnmbt");
		return;
	}
	m = p + 1;
	*p = 0;
	p = strchr(m, ' ');
	if(!p){
		ERROR("corrupt input: use Wnmbt");
		return;
	}
	s = atoi(p + 1);
	*p = 0;
	p = strchr(p+1, ' ');
	if(!p){
		ERROR("corrupt input: use Wnmbt");
		return;
	}
	t = atoi(p + 1);
	
	a = new Article(m, l, s, t);

	// insert new article into proper channel
	chanlst->insert(a);
}


// our main sludge pumping loop
void sludgepump(){
	Channel *c;
	char *b;
	fd_set rfds, wfds, xfds;
	struct timeval to;
	int i, t, now, na;
	int lasteval, nexteval;
	int closing=0, alldone=0;
	

	gettimeofday(&currenttime, 0);
	now = NOW;
	lasteval = now;
	nexteval = now + CHAN_EVAL_TIME;
	
	while(1){
		
		// populate fdsets
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&xfds);
		t = 0;
		na = 0;
		c = chanlst;
		while(c){
			if( c->wantr && c->io )
				FD_SET( c->io->fd, &rfds);
			if( c->wantw && c->io )
				FD_SET(c->io->fd, &wfds);
			if( ! c->privstate )
				na = 1;
			if( c->timeout && c->io )
				t = t ? MIN(t, c->timeout) : c->timeout;

			c = c->next;
			
		}
		FD_SET( inn->fd, &rfds);
		
		if( t || na ){
			// if something wants a and has a, then just effect a poll
			now = NOW;
			to.tv_sec = na ? 0 : (t - now);
			if( to.tv_sec < 0 )
				to.tv_sec = 0;
			to.tv_usec = 0;
			
			i = select(FD_SETSIZE, &rfds, &wfds, &xfds, &to);
		}else{
			i = select(FD_SETSIZE, &rfds, &wfds, &xfds, 0);
		}
		if( i < 0 ){
			if( errno == EINTR )
				continue;

			ERROR( sys_errlist[ errno ] );
		}

		gettimeofday(&currenttime, 0);
		
		// anything available from innd?
		if( FD_ISSET(inn->fd, &rfds)){
			while( !inn->fill() ){
				while( (b=inn->getline())){
					newarticle(b);
				}
			}
		}
		if( inn->flags & NIO_EOF ){
			// inn has closed us down
			// cleanup up and exit
			// should we finish sending what we have, or backlog it all?
			// XXX - cleanup

			if( alldone )
				exit(0);
			
			if( closing++ > 100 )
				exit(0);

			alldone = 1;
		}
			
		
		// which channels can proceed?
		now = NOW;
		c = chanlst;
		while(c){
			bool rp, wp;

			rp = c->io ? FD_ISSET(c->io->fd, &rfds) : 0;
			wp = c->io ? FD_ISSET(c->io->fd, &wfds) : 0;
			if( rp || wp
			    || (c->wanta && c->incoming.narts && !c->wantr && !c->wantw )
			    || (c->timeout && c->timeout < now) || !c->privstate
			    || ( closing && c->wanta && !c->wantr && !c->wantw ) ){
				if( c->statef )
					c->statef(c, rp, wp, c->incoming.narts);
				else{
					// connection has been shutdown
					// what to do? what to do?
					if( closing )
						alldone = 0;
				}
			}
			// log ?
			if( c->stats.offered >= logevery )
				c->log();
			
			c = c->next;
		}
#if 0		
		if( now > nexteval ){
			int dt;
			float ds;
			float pctidle, v;
			
			c = chanlst;
			dt = now - lasteval;
			ds = 0;
			while(c){
				if( c->stats.e_idleflag ){
				        // channel is currently idle
					c->stats.e_idletime += now - c->stats.e_idlestart;
					c->stats.e_idlestart = now;
				}
				pctidle = c->stats.e_idletime / (float)dt * 100.0;
				if( !c->stats.e_times )
					pctidle = 99;
				ds = c->reeval(ds, dt, pctidle);
				c->stats.e_clear();
				c = c->next;
			}
			lasteval = now;
			nexteval = now + CHAN_EVAL_TIME;
		}
#endif		
		
	}
}

