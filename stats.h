
// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details
// $Id: stats.h,v 1.1.1.1 1998/01/19 15:46:46 jaw Exp $

#ifndef _stats_h_
#define _stats_h_

class Stats {
public:
	// stats since last logged
	int connects, offered, accepted, rejected, refused, deferred;
	int passed, tossed;
	int bytessent;
	int timelastlogged;

	// should we keep total stats (since program start)?
	
	// stats since last evaluation, used for evaluation
	int e_offered, e_accepted;
	int e_bytessent, e_minsize, e_maxsize;
	int e_times;
	int e_idletime, e_idlestart, e_idleflag;

	// for PID
	float pid_i, pid_d, pid_p, pid_prev;
	
	Stats();
	void e_clear();
};

#endif /* _stats_h_ */
