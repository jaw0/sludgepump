

// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details

#ifdef RCSID
static const char *const rcsid
= "@(#)$Id: channel.cc,v 1.1.1.1 1998/01/19 15:46:45 jaw Exp $";
#endif


#include <conf.h>
#include <buffer.h>
#include <channel.h>
#include <article.h>
#include <slopbucket.h>
#include <misc.h>

#include <syslog.h>
#include <sys/time.h>

Channel *chanlst = 0;

extern float k1, k2, k3;
extern timeval currenttime;
extern int maxbufferarts;

Channel::Channel(){

	number = 0;
	next = prev = 0;
	io = 0;
	privstate = 0;
	statef = nntpopen;
	maxsize = 0;
	timeout = 0;
	wanta = wantr = wantw = 0;

	statef(this, 0, 0, 0);	// start it up
}

void Channel::insert(Article *art){

	if( (maxsize && art->size > maxsize)
	    || (incoming.narts > maxbufferarts)
	    || (checked.narts > maxbufferarts)
	    || (outgoing.narts > maxbufferarts)){
		// hand it to next channel
		if( next ){
			next->insert(art);
			stats.passed ++;
		}else{
			backlog->insert(art);
			stats.tossed ++;
		}
		return;
	}
	
	DEBUG(1, "%d new article %s\n", number, art->mid);
	incoming.insert(art);

	// notify protocol engine of article
	if( wanta && !wantr && !wantw && statef )
		statef(this, 0, 0, 1);
	
}

float Channel::reeval(float ds, int dt, float pctidle){
	float err, v;
	int ns;
	
	err = pctidle - TARGET_IDLE_PCT;
	if( err < 0 )
		err *= (100.0 / TARGET_IDLE_PCT) - 1;
	
	stats.pid_p = err;
	stats.pid_d = (err - stats.pid_prev) / dt;
	stats.pid_i += (err + stats.pid_prev) * dt / 2;
	stats.pid_prev = err;

	if( stats.pid_i > PID_MAX_I )
		stats.pid_i = PID_MAX_I;
	
	v = k1 * stats.pid_p + k2 * stats.pid_i + k3 * stats.pid_d;

	ns = (int)(maxsize + v);
	
	DEBUG(1, "reeval %d: dt=%d pi=%.2f e=%f p=%f i=%f d=%f v=%f sz=%d->%d\n",
	      number, dt, pctidle, err, stats.pid_p, stats.pid_i, stats.pid_d, v, maxsize, ns);
	DEBUG(1, "reeval %d: inlen=%d offered=%d accepted=%d min=%d max=%d bytes=%d\n",
	      number, incoming.narts, stats.e_offered, stats.e_accepted,
	      stats.e_minsize, stats.e_maxsize, stats.e_bytessent);
	
	syslog(LOG_INFO, "reeval %s %d: dt=%d pi=%.2f e=%f p=%f i=%f d=%f v=%f sz=%d->%d\n",
	      hostname, number, dt, pctidle, err, stats.pid_p, stats.pid_i, stats.pid_d, v, maxsize, ns);
	syslog(LOG_INFO, "reeval %s %d: inlen=%d offered=%d accepted=%d min=%d max=%d bytes=%d\n",
	      hostname, number, incoming.narts, stats.e_offered, stats.e_accepted,
	      stats.e_minsize, stats.e_maxsize, stats.e_bytessent);
	
	if( !next ){
		if( v > 0 ){
			// last channel has idle
			// perhaps close channel down?
			
		}else{
			// last channel not idle
			// perhaps open another?
		}
	}

	// apply discounted carryover
	// maxsize += ds / 2;

	if( prev && ns < prev->maxsize + 10 )
		ns = prev->maxsize + 10;
	if( next && ns > next->maxsize )
		ns = ns + next->maxsize / 2;
	
	if( ns < 100 )
		ns = 100;
	if( ns > 10000000 )
		ns = 10000000;

	
	v = ns - maxsize;
	maxsize = ns;
	return v;
}

void Channel::log(){
	int t, dt;
	char buffer[1024];

	t = NOW;
	dt = t - stats.timelastlogged;
	if( !dt ) dt = 1;
	
	sprintf(buffer,
		"%s %d stats %d offered %d accepted %d rejected %d failed "
		"%d connects %d bytes %d secs %d passed %d tossed %.2f oaps %.2f aaps %.2f bps\n",
		hostname, number, stats.offered, stats.accepted, stats.rejected,
		stats.refused, stats.connects, stats.bytessent, dt,
		stats.passed, stats.tossed,
		stats.offered / (float)dt, stats.accepted / (float)dt,
		stats.bytessent / (float)dt);

	DEBUG(1, buffer);
	syslog(LOG_NOTICE, buffer);

	stats.connects = stats.offered = stats.accepted = stats.rejected
		= stats.refused = stats.deferred = 0;
	stats.bytessent = 0;
	stats.passed = stats.tossed = 0;
	stats.timelastlogged = t;

	sprintf(buffer, "%s %d: sz=%d buflen=%d,%d,%d,%d\n",
		hostname, number, maxsize, incoming.narts, checked.narts,
		outgoing.narts, sent.narts);

	DEBUG(1, buffer);
	syslog(LOG_NOTICE, buffer);
	
}
