

// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details

#ifdef RCSID
static const char *const rcsid
= "@(#)$Id: stats.cc,v 1.1.1.1 1998/01/19 15:46:47 jaw Exp $";
#endif


#include <conf.h>
#include <stats.h>
#include <misc.h>

#include <stdio.h>
#include <syslog.h>

Stats::Stats(){

	
	connects = offered = accepted = rejected = refused = deferred = 0;
	bytessent = 0;
	passed = tossed = 0;
	timelastlogged = time(0);
	
	e_offered = e_accepted = 0;
	e_bytessent = e_minsize = e_maxsize = 0;
	e_idletime = e_idlestart = e_idleflag = 0;
	e_times = 0;
	
	pid_i = pid_d = pid_p = pid_prev = 0;
}

void Stats::e_clear(){
	e_offered = e_accepted = 0;
	e_bytessent = e_minsize = e_maxsize = 0;
	e_idletime = 0;
	// do not clear e_idlestart or e_idleflag
	e_times = 0;
}

