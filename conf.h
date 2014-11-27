

// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details
// $Id: conf.h,v 1.1.1.1 1998/01/19 15:46:46 jaw Exp $


#define USE_MMAP	1

// approx rawpart rollover period
// this must be set lower than the rawpart rollover time
#define ROLLOVER_TIME	(60 * 60)

// pause before attempting to reopen connection after an error
#define ERROR_REOPEN_TIME 	10

// timeout when opening
#define NNTP_OPEN_TIMEOUT 	120

// timout waiting for responses
#define NNTP_RESP_TIMEOUT 	120

// timeout waiting to send
#define NNTP_SEND_TIMEOUT	120

// give up after this many errors
#define QUIT_AFTER_ERRORS	20

// re-evaluate channel params this often
#define CHAN_EVAL_TIME		180

#define TARGET_IDLE_PCT		20
#define TARGET_IDLE_FUZZ	5
#define Q_FACTOR		0.5

#define DEFAULT_K1		7.2
#define DEFAULT_K2		.00667
#define DEFAULT_K3		37.5

#define PID_MAX_I		100000

// this is not what you think this it is
// do not change
#define CHECK_AT_A_TIME		10

// override with -B
#define MAX_BUFFER_ARTS		1000

typedef unsigned long u_long;
typedef unsigned short u_short;


