#include <u.h>
#include <libc.h>
#include <thread.h>
#include "cpu.h"

int debug = 0;
int dpid = 0;
int dfpid = 0;
Channel *diochan;
Channel *dfreechan;

void
dfreeworker(void*)
{
	void *ptr;
	for(;;){
		ptr = recvp(dfreechan);
		if(ptr)
			free(ptr);
	}
}

void
dprintworker(void*)
{
	char *str;
	for(;;){
		str = recvp(diochan);
		if(str){
			fprint(2, "debug: %s\n", str);
			sendp(dfreechan, str);
		}
	}
}

void
startdworkers(void)
{
	if(dpid == 0){
		diochan = chancreate(sizeof(char*), 10);
		dpid = proccreate(dprintworker, nil, mainstacksize);
	}
	if(dfpid == 0){
		dfreechan = chancreate(sizeof(void*), 10);
		dfpid = proccreate(dfreeworker, nil, mainstacksize);
	}
}

void
dprint(char *str)
{
	char *cstr;

	if(debug){
		if(dpid == 0 || dfpid == 0)
			startdworkers();
		cstr = strdup(str);
		if(cstr == nil)
			panic("bad malloc");
		sendp(diochan, str);
	}
}

void
panic(char *s)
{
	fprint(2, "panic: %s\n", s);
	threadexitsall(smprint("panic: %s", s));
}
