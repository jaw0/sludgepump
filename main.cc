

// Copyright 1998 OpNet Inc. <news@op.net>
// see the file "COPYRIGHT" for details

#ifdef RCSID
static const char *const rcsid
= "@(#)$Id: main.cc,v 1.1.1.1 1998/01/19 15:46:45 jaw Exp $";
#endif


#include <conf.h>
#include <channel.h>
#include <nio.h>
#include <slopbucket.h>
#include <misc.h>

#include <stdio.h>
#include <syslog.h>
#include <signal.h>

extern "C" {
#include <configdata.h>
#include <raw.h>
};

extern char *optarg;
extern int optind;


char *hostname = 0;
u_long hostip = 0;
u_short hostport = 0;

NIO *inn = 0;		// feed from innd

int maxchans = 0;	// maximum open channels
int currchans = 0;	// current open channels
int debuglevel = 0;
int retrydefers = 0;
int logevery = 250;
int donotstream = 0;
int maxbufferarts = MAX_BUFFER_ARTS;

char *slopfile = 0;

FILE *dbgfp = 0;

float k1, k2, k3;
extern "C" {
extern double atof(const char*);
}

extern void sludgepump(void);
extern const char *const version(void);

void setthreshs(Channel *c, int nchan);
void sig_debug_up(int), sig_debug_off(int);


void usage(){

	fprintf(stderr, "usage: do something other than what you did\n");
}
	
int main(int argc, char **argv){
	int c, i;
	int initchan;
	Channel *lch;
	
	maxchans = 0;
	initchan = 0;
	debuglevel = 0;
	retrydefers = 0;
	hostport = 119;

	k1 = DEFAULT_K1;
	k2 = DEFAULT_K2;
	k3 = DEFAULT_K3;

	signal(SIGUSR1, sig_debug_up);
	signal(SIGUSR2, sig_debug_off);
	
	openlog("sludgepumper", LOG_PID, LOG_NEWS);
	
	while( (c=getopt(argc, argv, "b:B:d:D:i:l:m:nP:rv")) != -1){

		switch(c){

		  case 'b':		// backlog file
			slopfile = optarg;
			break;

		  case 'B':
			maxbufferarts = atoi( optarg );
			break;
			
		  case 'd':
			debuglevel = atoi( optarg );
			break;

		  case 'D':
			dbgfp = fopen(optarg, "a");
			if(!dbgfp)
				fprintf(stderr, "could not open debug file '%s'\n", optarg);
			break;

		  case 'i':
			initchan = atoi( optarg );
			break;

		  case 'l':
			logevery = atoi( optarg );
			break;
			
		  case 'm':
			maxchans = atoi( optarg );
			break;

		  case 'n':
			donotstream = 1;
			break;

		  case 'P':
			hostport = atoi( optarg );
			break;
			
		  case 'r':
			retrydefers = 1;
			break;

		  case 'v':
			printf("Good morning, Dr. Chandra.\nI am %s\n", version());
			exit(0);

		}
	}

	if( !dbgfp )
		dbgfp = stderr;
	
	argc -= optind;
	argv += optind;

	if( argc < 1 ){
		usage();
		exit(-1);
	}
	hostname = *argv;

	hostip = get_ipaddr(hostname);
	if(!hostip){
		sleep(1);
		hostip = get_ipaddr(hostname);
	}
	if(!hostip){
		fprintf(stderr, "No such host '%s'\n", hostname);
		exit(-1);
	}

	if( !maxchans ){
		maxchans = 10;
		fprintf(stderr, "no maxchans specified using default of %d\n", maxchans);
	}
	if( ! initchan )
		initchan = (maxchans + 1) / 2;


	// set up io from inn (stdin)
	inn = new NIO( STDIN_FILENO );
	SetNonBlocking( inn->fd );

	// set up backlog file
	backlog = new SlopBucket(slopfile);
	
	chanlst = lch = 0;
	for(i=0; i<initchan; i++){
		Channel *ch;

		ch = new Channel;
		ch->prev = lch;
		if( lch ){
			lch->next = ch;
		}else{
			chanlst = ch;
		}
		lch = ch;

		ch->maxsize = 512 * (1<<i);
		ch->number = i;
	}
	setthreshs(chanlst, initchan);
	// last channel gets way-big initial maxsize
	lch->maxsize = 1000000;

	RAWread_config();
	RAWinitdisks(0);
	
	sludgepump();
}

#define S_LOW	300
#define S_HIGH	30000
#define S_STEP	5

void setthreshs(Channel *c, int nchan){
	float cum[S_HIGH/S_STEP];
	int i;
	int chn;
	float x, xx, y;

	cum[S_LOW-1] = 0;
	for(i=S_LOW; i<S_HIGH; i+=S_STEP){
		// no, you may not steal this piece of code without
		// properly attributing it
		x = (i - S_LOW)/1000.0;
		xx = x * x;
		y = 0.21 * x / ( xx * xx + 2 * xx + 1);
		cum[i/S_STEP] = y * i + cum[i/S_STEP - 1];

	}

	chn = 1;
	for(i=S_LOW; i<S_HIGH; i+=S_STEP){
		if( cum[i/S_STEP] <= chn * cum[S_HIGH/S_STEP-1] / nchan)
			c->maxsize = i;
		else{
			DEBUG(1, "chan %d size %d\n", c->number, c->maxsize);
			chn++;
			c = c->next;
		}
	}
}

void sig_debug_up(int s){
	debuglevel ++;
}
void sig_debug_off(int s){
	debuglevel = 0;
}

