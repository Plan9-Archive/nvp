#include <u.h>
#include <libc.h>
#include <thread.h>
#include "cpu.h"

typedef struct {
	char *str;
	int pid;
	int cmd;
} Msg;

int debug = 0;
int dpid = 0;
int dfpid = 0;
int mpid = 0;
Channel *diochan;
Channel *dfreechan;
Channel *allocchan;

void
dfreeworker(void*)
{
	Msg *ptr;

	threadsetname("nvp vm dfree worker");
	for(;;){
		ptr = recvp(dfreechan);
		free(ptr->str);
		free(ptr);
	}
}

void
dprintworker(void*)
{
	Msg *m;

	threadsetname("nvp vm dprint worker");
	for(;;){
		m = recvp(diochan);
		if(m){
			if(m->cmd == 1){
				fprint(2, "panic %d: %s\n", m->pid, m->str);
				threadexitsall("panic");
			}else if(m->cmd == 2){
				fprint(2, "%s", m->str);
				sendp(dfreechan, m);
			} else {
				fprint(2, "debug %d: %s\n", m->pid, m->str);
				sendp(dfreechan, m);
			}
		}
	}
}

void
allocworker(void*)
{
	Msg *m;

	threadsetname("nvp vm dalloc worker");
	for(;;){
		m = malloc(sizeof(Msg));
		if(!m)
			panic("bad malloc");
		sendp(allocchan, m);
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
	if(mpid == 0){
		allocchan = chancreate(sizeof(void*), 10);
		mpid = proccreate(allocworker, nil, mainstacksize);
	}
}

void
dprint(char *str)
{
	Msg *m;

	if(debug){
		if(dpid == 0 || dfpid == 0)
			startdworkers();
		m = recvp(allocchan);
		if(!m){
			debug = 0;
			panic("bad malloc");
		}
		m->pid = getpid();
		m->str = strdup(str);
		if(m->str == nil){
			debug = 0;
			panic("bad malloc");
		}
		m->cmd = 0;
		sendp(diochan, m);
	}
//	free(str);		// smprint mallocs things moron. its what the m means!
}

void
panic(char *s)
{
	Msg *m;

	if(debug){
		if(dpid == 0 || dfpid == 0)
			startdworkers();
		m = recvp(allocchan);
		if(!m){
			debug = 0;
			panic("bad malloc");
		}
		m->pid = getpid();
		m->str = strdup(s);
		if(m->str == nil){
			debug = 0;
			panic("bad malloc");
		}
		m->cmd = 1;
		sendp(diochan, m);
		for(;;)
			sleep(1);
	} else {
		fprint(2, "panic %d: %s\n", getpid(), s);
		threadexitsall(smprint("panic: %s", s));
	}
}
