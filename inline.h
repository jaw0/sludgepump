
// Copyright 1998 OpNet Inc. <news@op.net>


inline int hash(const char *p){
	int hv = 0;

	while(*p)
		hv = hv<<1 ^ *p++;
        hv = hv<0 ? -hv : hv;
        return hv;
}

inline void getresponse(char *l, Response *r){
	char *p;

	r->msg = 0;
	r->len = 0;

	p = strchr(l, ' ');
	if(!p) p = strchr(l, '\t');	// be lenient in what you accept
	
	if( p ){
		*p = 0;
		r->msg = p + 1;
		r->len = strlen(r->msg);
	}

	r->code = atoi( l );
}

