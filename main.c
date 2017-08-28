#include <u.h>
#include <libc.h>
#include <thread.h>
#include "cpu.h"
#include "devs.h"

Cpu *cpu0;
int haltonbp;
int printonbp;

char *regnames[] = {
	// gp registers
	[R0] "R0", [R1] "R1", [R2] "R2", [R3] "R3",
	[R4] "R4", [R5] "R5", [R6] "R6", [R7] "R7",
	// gp/args registers
	[S0] "S0", [S1] "S1", [S2] "S2", [S3] "S3",
	[S4] "S4", [S5] "S5", [S6] "S6", [S7] "S7",
	// vector registers
	[V0] "V0", [V1] "V1", [V2] "V2", [V3] "V3",
	[V4] "V4", [V5] "V5", [V6] "V6", [V7] "V7",
	// other register
	[OP] "OP",
	[PC] "PC",
	[SP] "SP",
	[SY] "SY",
	[FL] "FL",
	nil,
};

void
doimplop(Cpu *c, Inst *inst)
{
	char *s;
	u32int *sreg, addr;
	int i;
	Vect *vreg;

	switch(inst->op){
	case 0x10: // bp
		if(haltonbp)
			c->pause = 1;
		if(printonbp)
			dumpregs(cpu0);
		break;
	case 0x11: // print
 		addr = inst->args[0];
		s = (char*)&memory[addr];
		print("%s", s);
		break;
	case 0x12: // rfmt
		if((inst->args[0] >= R0) && (inst->args[0] <= S7)){
			sreg = getregister(c, inst->args[0]);
			print("%s = %ux\n", regnames[inst->args[0]], *sreg);
		} else if((inst->args[0] >= V0) && (inst->args[0] <= V7)) {
			vreg = getvreg(c, inst->args[0]);
			print("%s = { ", regnames[inst->args[0]]);
			for(i = 0; i < VECLEN; i++)
				print("%ux ", vreg->dat[i]);
			print("}\n");
		} else {
			sreg = getregister(c, inst->args[0]);
			print("%s = %ux\n", regnames[inst->args[0]], *sreg);
		}
		break;
	}
}

void
usage(void)
{
	print("usage: nvp [-dbp] [-m memlen] [-s start] [-i cycle limit] [-l disk]  -f bin\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	u32int mlen = 1048576, start = 0, ilimit = 0, disklen;
	char *fname = nil;
	char *diskname = nil;
	int fd, dfd;
	vlong flen;

	haltonbp = 0;
	ARGBEGIN{
	case 'f':
		fname = strdup(EARGF(usage()));
		break;
	case 'm':
		mlen = atoi(EARGF(usage()));
		break;
	case 's':
		start = atoi(EARGF(usage()));
		break;
	case 'i':
		ilimit = atoi(EARGF(usage()));
		break;
	case 'l':
		diskname = strdup(EARGF(usage()));
		break;
	case 'd':
		debug = 1;
		break;
	case 'b':
		haltonbp = 1;
		break;
	case 'p':
		printonbp = 1;
		break;
	case 'h':
		usage();
		break;
	default:
		fprint(2, "nvp: unknown arg\n");
		usage();
		break;
	}ARGEND;
	
	if(fname == nil)
		usage();
	fd = open(fname, OREAD);
	if(fd < 0)
		panic("error opening boot image");
	flen = seek(fd, 0, 2);
	seek(fd, 0, 0);
	if(diskname != nil){
		dfd = open(diskname, OREAD|OWRITE);
		if(dfd < 0)
			panic("could no open disk");
		disklen = seek(dfd, 0, 2);
		seek(dfd, 0, 0);
		initdisk(dfd, disklen);
	}

	meminit(mlen);
	vlong rst = readn(fd, &memory[start], flen);
	if(rst < flen)
		panic(smprint("short read: %ulld < %ulld: %r\n", rst, flen));
	consinit();

	cpu0 = makecpu();
	cpu0->ilimit = ilimit;
	startvectworker(cpu0);
	startexec(cpu0, start);
	// should never get here
	threadexitsall("what?");
}
