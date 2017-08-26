#include <u.h>
#include <libc.h>
#include <thread.h>
#include "cpu.h"
#include "devs.h"

enum {
	DREAD = 0x10,
	DWRITE = 0x11,
};

typedef struct {
	int op;
	u32int memaddr;
	u32int diskaddr;
	u8int len;
} diskop;

Channel *dop;
Channel *reply;
int dfd;
u32int disklen;

void
diskworker(void*)
{
	u8int *buf;
	long st;
	diskop *op;

	buf = malloc(sizeof(u8int)*257);
	threadsetname("nvp disk worker");
	if(!buf)
		panic("could not malloc disk buffer");
	for(;;){
		op = recvp(dop);
		switch(op->op){
		case DREAD:
			if(op->memaddr >= memlen)
				panic("disk: bad memory write");
			st = pread(dfd, buf, op->len, op->diskaddr);
			if(st != op->len)
				panic("short read");
			memcpy(&memory[op->memaddr], buf, op->len);
			break;
		case DWRITE:
			if(op->memaddr >= memlen)
				panic("disk: bad memory read");
			memcpy(buf, &memory[op->memaddr], op->len);
			st = pwrite(dfd, buf, op->len, op->diskaddr);
			if(st != op->len)
				panic("short write");
			break;
		default:
			panic(smprint("unknown disk command %x", op->op));
			break;
		}
		send(reply, &st);
		free(op);
	}
}

u8int
diskstatus(void)
{
	long st;

	if(nbrecv(reply, &st) == 0)
		return 0;
	else
		return 1;
}

void
diskcmd(u8int oper)
{
	diskop *op;

	op = malloc(sizeof(diskop));
	if(op == nil)
		panic("bad malloc");
	op->diskaddr = disk.dat[0] << 24 | disk.dat[1] << 16 | disk.dat[2] << 8 | disk.dat[3];
	op->memaddr = disk.dat[4] << 24 | disk.dat[5] << 16 | disk.dat[6] << 8 | disk.dat[7];
	op->len = disk.dat[8];
	op->op = oper;
	sendp(dop, op);
}

void
initdisk(int fd, u32int len)
{
	disklen = len;
	dfd = fd;
	dop = chancreate(sizeof(uintptr), 5);
	reply = chancreate(sizeof(long), 5);
	proccreate(diskworker, nil, mainstacksize);
	if(!dop || !reply)
		panic("bad chancreate");
}

Device disk = {
	.active = 1,
	.init = nil,
	.read = nil,
	.write = nil,
	.status = diskstatus,
	.cmd = diskcmd,
};
