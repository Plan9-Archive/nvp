#include <u.h>
#include <libc.h>
#include <thread.h>
#include "cpu.h"

Cpu *cpu0;

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
		c->pause = 1;
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
	print("usage: nvp -f bin [-m memlen] [-s start]\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	u32int mlen = 1048576, start = 0;
	char *fname = nil;
	int fd;
	vlong flen;

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
	case 'd':
		debug = 1;
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
	if(fd < 0){
		fprint(2, "nvp: error opening binary\n");
		threadexitsall("file open");
	}
	flen = seek(fd, 0, 2);
	seek(fd, 0, 0);

	meminit(mlen);
	vlong rst = readn(fd, &memory[start], flen);
	if(rst < flen){
		fprint(2, "nvp: short read: %ulld < %ulld: %r\n", rst, flen);
		threadexitsall("short read");
	}

	cpu0 = makecpu();
	startvectworker(cpu0);
	startexec(cpu0, start);
	// should never get here
	threadexitsall("what?");
}
