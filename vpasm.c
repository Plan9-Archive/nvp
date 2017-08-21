#include <u.h>
#include <libc.h>
#include <bio.h>
#include "regs.h"
#include "inst.h"
#include "vpasm.h"

int maxinst = 4;
int debug = 0;
int linen = 0;
u32int addrcnt = 0;
Inst *src;
Label *labels;
Instd *output;
Data *datas;
char *panicfn;

Instdef insts[] = {
	// scalar mem
[0]		{"loads", CATSMEM, LOADS, 7},
[1]		{"stores", CATSMEM, STORES, 7},
[2]		{"moves", CATSMEM, MOVES, 4},
[3]		{"rloads", CATSMEM, RLOADS, 4},
[4]		{"rstores", CATSMEM, RSTORES, 4},
[5]		{"push", CATSMEM, PUSH, 3},
[6]		{"pop", CATSMEM, POP, 3},
[7]		{"loadc", CATSMEM, LOADC, 7},
	// vector mem
[8]		{"clrs", CATSMEM, CLRS, 3},
[9]		{"loadv", CATVMEM, LOADV, 7},
[10]	{"storev", CATVMEM, STOREV, 7},
[11]	{"movev", CATVMEM, MOVEV, 4},
[12]	{"rloadv", CATVMEM, RLOADV, 4},
[13]	{"rstorev", CATVMEM, RSTOREV, 4},
[14]	{"rloadsv", CATVMEM, RLOADSV, 5},
[15]	{"rstoresv", CATVMEM, RSTORESV, 5},
[16]	{"clrv", CATVMEM, CLRV, 3},
	// control flow
[17]	{"nop", CATCTL, NOP, 2},
[18]	{"jmp", CATCTL, JMP, 6},
[19]	{"rjmp", CATCTL, RJMP, 6},
[20]	{"jmpr", CATCTL, JMPR, 3},
[21]	{"rjmpr", CATCTL, RJMPR, 3},
[22]	{"clrf", CATCTL, CLRF, 2},
[23]	{"cjmp", CATCTL, CJMP, 6},
[24]	{"crjmp", CATCTL, CRJMP, 6},
[25]	{"cjmpr", CATCTL, CJMPR, 3},
[26]	{"crjmpr", CATCTL, CRJMPR, 3},
[27]	{"setsys", CATCTL, SETSYS, 6},
[28]	{"syscall", CATCTL, SYSCALL, 2},
[29]	{"call", CATCTL, CALL, 6},
[30]	{"rcall", CATCTL, RCALL, 6},
[31]	{"return", CATCTL, RETURN, 2},
[32]	{"halt", CATCTL, HALT, 2},
	// scalar math
[33]	{"eqs", CATSMATH, EQS, 4},
[34]	{"gts", CATSMATH, GTS, 4},
[35]	{"lts", CATSMATH, LTS, 4},
[36]	{"adds", CATSMATH, ADDS, 5},
[37]	{"subs", CATSMATH, SUBS, 5},
[38]	{"muls", CATSMATH, MULS, 5},
[39]	{"divs", CATSMATH, DIVS, 5},
[40]	{"mods", CATSMATH, MODS, 5},
[41]	{"nands", CATSMATH, NANDS, 5},
	// vector math
[42]	{"eqv", CATVMATH, EQV, 4},
[43]	{"gtv", CATVMATH, GTV, 4},
[44]	{"ltv", CATVMATH, LTV, 4},
[45]	{"addv", CATVMATH, ADDV, 5},
[46]	{"subv", CATVMATH, SUBV, 5},
[47]	{"mulv", CATVMATH, MULV, 5},
[48]	{"divv", CATVMATH, DIVV, 5},
[49]	{"modv", CATVMATH, MODV, 5},
[50]	{nil, 0, 0, 0},
};

char *regs[] = {
	[R0] "r0", [R1] "r1", [R2] "r2", [R3] "r3", [R4] "r4", [R5] "r5",
	[R6] "r6", [R7] "r7",
	[S0] "s0", [S1] "s1", [S2] "s2", [S3] "s3", [S4] "s4", [S5] "s5",
	[S6] "s6", [S7] "s7",
	[V0] "v0", [V1] "v1", [V2] "v2", [V3] "v3", [V4] "v4", [V5] "v5",
	[V6] "v6", [V7] "v7",
	[OP] "op", [PC] "pc", [SP] "sp", [FL] "fl", [SY] "sy",
};

void
panic(char *arg)
{
	fprint(2, "vpasm (line %d): error: %s: %s: %r\n",linen , panicfn, arg);
	exits(panicfn);
}

void*
emalloc(uvlong sz)
{
	E("emalloc");
	void *v;

	v = malloc(sz);
	if(!v)
		panic("bad malloc");
	DEBUG(smprint("v = %p", v));
	DEBUG("setmalloctag");
	setmalloctag(v, getcallerpc(&sz));
	DEBUG("memset");
	memset(v, 0, sz);
	return v;
}

Inst*
getinstr(char *str)
{
	E("getinstr");
	char *sline[4], *tmp;
	int slen, i;
	Inst *inst;

	inst = emalloc(sizeof(Inst));
	inst->raw = strdup(str);
	tmp = strdup(str);
	if(inst->raw == nil || tmp == nil)
		panic("bad malloc");
	slen = tokenize(tmp, sline, maxinst);
	inst->slen = slen;
	inst->inst = sline[0];
	inst->args = emalloc(sizeof(char*)*(slen-1));
	for(i = 1; i < slen; i++)
		inst->args[i-1] = sline[i];
	normalizeinst(inst);
	return inst;
}

void
normalizeinst(Inst *inst)
{
	E("normalizeinst");
	int i;

	for(i = 0; i < 52; i++){
		DEBUG(smprint("inst %s, checking with %s", inst->inst, insts[i].name));
		if(strcmp(inst->inst, insts[i].name) == 0){
			inst->len = insts[i].len;
			return;
		}
	}
	panic(smprint("unknown instruction %s: ending", inst->inst));
}

u32int
labeltoaddr(char *str)
{
	E("labeltoaddr");
	Label *cur;

	cur = labels;
	if(!cur)
		panic("no labels");
	while(cur != nil){
		if(strcmp(cur->name, str) == 0)
			return cur->addr;
		cur = cur->next;
	}
	panic(smprint("could not find label %s", str));
	return 0;
}

u32int
decodeaddr(char *str)
{
	E("decodeaddr");
	if(str[0] == '*')
		return labeltoaddr(&str[1]);
	return atoi(str);
}

void
addlabel(char *name, u32int addr)
{
	E("addlabel");
	Label *cur;

	cur = labels;
	while(cur->next != nil)
		cur = cur->next;
	cur->next = emalloc(sizeof(Label));
	cur = cur->next;
	cur->name = strdup(name);
	cur->addr = addr;
}

void
handleinst(char *str)
{
	E("handleinst");
	Inst *inst;
	Inst *cur;

	inst = getinstr(str);
	if(src != nil){
		cur = src;
		while(cur->next != nil)
			cur = cur->next;
		cur->next = inst;
	} else
		src = inst;
	addrcnt += inst->len;
}

void
adddata(u32int dat)
{
	Data *cur = datas;

	if(cur == nil){
		datas = emalloc(sizeof(Data));
		cur = datas;
	} else {
		while(cur->next != nil)
			cur = cur->next;
		cur->next = emalloc(sizeof(Data));
		cur = cur->next;
	}
	cur->addr = addrcnt;
	cur->dat = dat;
	cur->next = nil;
	addrcnt += sizeof(u32int);
}

void
handlepreproc(char *str)
{
	E("handlepreproc");
	char *toks[9];
	int toksl;
	int i;

	toksl = tokenize(str, toks, 9);
	if(strcmp(".start", toks[0]) == 0){
		addrcnt = 0;
		return;
	}
	if(strcmp(".label", toks[0]) == 0){
		addlabel(toks[1], addrcnt);
		return;
	}
	if(strcmp(".data", toks[0]) == 0){
		adddata(atoi(toks[1]));
		return;
	}
	if(strcmp(".vdata", toks[0]) == 0){
		toksl--;
		for(i = 0; i < toksl; i++)
			adddata(atoi(toks[i+1]));
		return;
	}
}

int
readsrcline(Biobuf *bf)
{
	E("readsrcline");
	char *str;

	str = Brdstr(bf, '\n', 1);
	linen++;
	if(!str)
		return -1;
	if(strcmp(str, ".eof") == 0)
		return -2;
	if(str[0] == ';')
		return 0;
	if(str[0] == '.'){
		handlepreproc(str);
	} else {
		handleinst(str);
	}
	return 0;
}

Instdef*
getinstdef(char *op)
{
	E("getinstdef");
	int i;

	for(i = 0;;i++){
		if(insts[i].len == 0)
			panic("instruction not found");
		if(strcmp(insts[i].name, op) == 0)
			return &insts[i];
	}
}

u8int
getreg(char *str)
{
	E("getreg");
	int i;
	u8int rval = 0;

	for(i = 0; i < SY+1; i++){
		if(regs[i] == nil)
			continue;
		if(strcmp(str, regs[i]) == 0){
			rval = i;
			break;
		}
	}
	if(rval == 0)
		panic("unknown register");
	return rval;
}

void
encode32int(u32int src, u8int *dest)
{
	dest[0] = src >> 24 & 0xff;
	dest[1] = src >> 16 & 0xff;
	dest[2] = src >> 8 & 0xff;
	dest[3] = src & 0xff;
}

Instd*
encodeinst(Inst *inst)
{
	E("encodeinst");
	Instd *einst;
	Instdef *def;
	int iaddr = 0, i;

	einst = emalloc(sizeof(Instd));
	def = getinstdef(inst->inst);
	einst->otype = 0;
	einst->type = def->type;
	einst->len = def->len;
	for(i = 0; i < inst->slen; i++){
		if(*inst->args[i] == 'r' ||
		 *inst->args[i] == 'v' ||
		 *inst->args[i] == 's' ||
		 *inst->args[i] == 'o' ||
		 *inst->args[i] == 'p' ||
		 strcmp(inst->args[i], "fl") == 0){
			einst->args[iaddr] = getreg(inst->args[i]);
			iaddr++;
		} else {
			encode32int(decodeaddr(inst->args[i]), &einst->args[iaddr]);
			iaddr += sizeof(u32int);
		}
	}
	return einst;
}

Instd*
encodedata(Data *d)
{
	E("encodedata");
	Instd *einst;

	einst = emalloc(sizeof(Instd));
	einst->otype = 1;
	encode32int(d->dat, &einst->dat[0]);
	return einst;
}

void
generateoutput(void)
{
	E("generateoutput");
	u32int curaddr;
	Inst *curinst;
	Data *curdat;
	Instd *curout;
	Instd *start;

	start = curout = emalloc(sizeof(Instd));
	curinst = src;
	curdat = datas;
	curaddr = addrcnt;
	for(;;){
		if(curdat != nil && curdat->addr == curaddr){
			curout->next = encodedata(curdat);
			curaddr += sizeof(u32int);
			curdat = curdat->next;
		} else if(curinst != nil){
			curout->next = encodeinst(curinst);
			curaddr += curinst->len;
			curinst = curinst->next;
		} else
			break;
		if(curinst->next == nil)
			panic("no data generated");
		curinst = curinst->next;
	}
	output = start->next;
}

int
dumpoutput(Biobuf *bf)
{
	E("dumpoutput");
	Instd *cur;
	u8int tmp;

	cur = output;
	while(cur != nil){
		switch(cur->otype){
		case 0:
			tmp = cur->type + cur->len;
			if(Bwrite(bf, &tmp, 1) < 1)
				goto fail;
			if(Bwrite(bf, &cur->inst, 1) < 1)
				goto fail;
			if(cur->len - 2 > 0)
				if(Bwrite(bf, &cur->args[0], (cur->len - 2)) < (cur->len - 2))
					goto fail;
			break;
		case 1:
			if(Bwrite(bf, &cur->dat[0], 4) < 4)
				goto fail;
			break;
		}
		cur = cur->next;
	}
	if(!Bflush(bf))
		panic("could not flush output file");
	return 0;
fail:
	panic("could not write instruction to file");
	return -1;
}

void
usage(void)
{
	fprint(2, "usage: vpasm [-dh] [-s src file] [-o outfile]\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	E("main");
	char *srcfile = nil, *outfile = nil;
	Biobuf *srcfd, *outfd;
	int st = 0;

	ARGBEGIN{
	case 'd':
		debug = 1;
		break;
	case 's':
		srcfile = strdup(EARGF(usage()));
		break;
	case 'o':
		outfile = strdup(EARGF(usage()));
		break;
	case 'h':
		usage();
		break;
	}ARGEND;

	if(!srcfile || !outfile)
		usage();

	srcfd = Bopen(srcfile, OREAD);
	outfd = Bopen(outfile, OWRITE);
	if(!srcfd || !outfd)
		panic("could not open src or output files");
	
	while(st != -1 || st != -2)
		st = readsrcline(srcfd);
	if(dumpoutput(outfd) != 0)
		panic("could not dump the assembled file");
	Bterm(srcfd);
	Bterm(outfd);
	exits(nil);
}
