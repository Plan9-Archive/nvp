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
	{"loads", CATSMEM, LOADS, 7},
	{"stores", CATSMEM, STORES, 7},
	{"moves", CATSMEM, MOVES, 4},
	{"rloads", CATSMEM, RLOADS, 4},
	{"rstores", CATSMEM, RSTORES, 4},
	{"push", CATSMEM, PUSH, 3},
	{"pop", CATSMEM, POP, 3},
	{"loadc", CATSMEM, LOADC, 7},
	// vector mem
	{"clrs", CATSMEM, CLRS, 3},
	{"loadv", CATVMEM, LOADV, 7},
	{"storev", CATVMEM, STOREV, 7},
	{"movev", CATVMEM, MOVEV, 4},
	{"rloadv", CATVMEM, RLOADV, 4},
	{"rstorev", CATVMEM, RSTOREV, 4},
	{"rloadsv", CATVMEM, RLOADSV, 5},
	{"rstoresv", CATVMEM, RSTORESV, 5},
	{"clrv", CATVMEM, CLRV, 3},
	// control flow
	{"nop", CATCTL, NOP, 2},
	{"jmp", CATCTL, JMP, 6},
	{"rjmp", CATCTL, RJMP, 6},
	{"jmpr", CATCTL, JMPR, 3},
	{"rjmpr", CATCTL, RJMPR, 3},
	{"clrf", CATCTL, CLRF, 2},
	{"cjmp", CATCTL, CJMP, 6},
	{"crjmp", CATCTL, CRJMP, 6},
	{"cjmpr", CATCTL, CJMPR, 3},
	{"crjmpr", CATCTL, CRJMPR, 3},
	{"setsys", CATCTL, SETSYS, 6},
	{"syscall", CATCTL, SYSCALL, 2},
	{"call", CATCTL, CALL, 6},
	{"rcall", CATCTL, RCALL, 6},
	{"return", CATCTL, RETURN, 2},
	{"halt", CATCTL, HALT, 2},
	// scalar math
	{"eqs", CATSMATH, EQS, 4},
	{"gts", CATSMATH, GTS, 4},
	{"lts", CATSMATH, LTS, 4},
	{"adds", CATSMATH, ADDS, 5},
	{"subs", CATSMATH, SUBS, 5},
	{"muls", CATSMATH, MULS, 5},
	{"divs", CATSMATH, DIVS, 5},
	{"mods", CATSMATH, MODS, 5},
	{"nands", CATSMATH, NANDS, 5},
	// vector math
	{"eqv", CATVMATH, EQV, 4},
	{"gtv", CATVMATH, GTV, 4},
	{"ltv", CATVMATH, LTV, 4},
	{"addv", CATVMATH, ADDV, 5},
	{"subv", CATVMATH, SUBV, 5},
	{"mulv", CATVMATH, MULV, 5},
	{"divv", CATVMATH, DIVV, 5},
	{"modv", CATVMATH, MODV, 5},
	// implementation specific
	{"bp", CATIMPL, BP, 2},
	{"print", CATIMPL, PRINT, 6},
	{"rfmt", CATIMPL, RFMT, 3},
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
	setmalloctag(v, getcallerpc(&sz));
	memset(v, 0, sz);
	return v;
}

void
dumpsrc(void)
{
	E("dumpsrc");
	Inst *cur;
	int i;

	if(src == nil){
		fprint(2, "no instructions to dump");
		return;
	}
	for(cur = src; cur != nil; cur = cur->next){
		fprint(2, "Inst (%p){\n", cur);
		fprint(2, "\tlen = %d,\n\tslen = %d,\n", cur->len, cur->slen);
		fprint(2, "\traw = '%s',\n", cur->raw);
		fprint(2, "\tinst = '%s',\n", cur->inst);
		fprint(2, "\targs = {\n");
		for(i = 0; i < (cur->slen-1); i++)
			fprint(2, "\t\t[%d] = %s,\n", i, cur->args[i]);
		fprint(2, "\t}\n");
		fprint(2, "\tnext = %p\n}\n\n", cur->next);
	}
}

void
dumpdata(void)
{
	E("dumpdata");
	Data *cur;

	if(datas == nil){
		fprint(2, "no data to dump");
		return;
	}
	for(cur = datas; cur != nil; cur = cur->next)
		fprint(2, "Data (%p){ addr = %ux, dat = %ux, next = %p }\n",
				cur, cur->addr, cur->dat, cur->next);
}

void
dumplabels(void)
{
	E("dumplabels");
	Label *cur;

	if(labels == nil){
		fprint(2, "no labels to dump\n");
		return;
	}
	for(cur = labels; cur != nil; cur = cur->next)
		fprint(2, "Label (%p){ name = %s, addr = %ux, next = %p }\n",
				cur, cur->name, cur->addr, cur->next);
}

void
dumpgens(void)
{
	E("dumpgens");
	Instd *cur;
	int i;

	if(output == nil){
		fprint(2, "no output to dump\n");
		return;
	}
	for(cur = output; cur != nil; cur = cur->next){
		fprint(2, "Instd (%p){\n", cur);
		switch(cur->otype){
		case 0:
			fprint(2, "\tInst@%x (%p){\n", cur->addr, cur->src);
			fprint(2, "\t\tlen = %d,\n\tslen = %d,\n", cur->src->len, cur->src->slen);
			fprint(2, "\t\traw = '%s',\n", cur->src->raw);
			fprint(2, "\t\tinst = '%s',\n", cur->src->inst);
			fprint(2, "\t\targs = {\n");
			for(i = 0; i < (cur->src->slen-1); i++)
				fprint(2, "\t\t\t[%d] = %s,\n", i, cur->src->args[i]);
			fprint(2, "\t\t}\n");
			fprint(2, "\t}\n");
			fprint(2, "\t{ type = %x, len = %x }(header = %x)\n",
					cur->type, cur->len, cur->len+cur->type);
			fprint(2, "\tinst = %x\n", cur->inst);
			fprint(2, "\targs = {\n");
			for(i = 0; i < cur->len-2; i++)
				fprint(2, "\t\t[%d] %x,\n", i, cur->args[i]);
			fprint(2, "\t}\n");
			break;
		case 1:
			fprint(2, "\tData (%p){ addr = %ux, dat = %ux, next = %p }\n",
					cur->datsrc, cur->datsrc->addr, cur->datsrc->dat, cur->datsrc->next);
			fprint(2, "\tdat = {\n");
			for(i = 0; i < 4; i++)
				fprint(2, "\t\t[%d] %x\n", i, cur->dat[i]);
			fprint(2, "\t}\n");
			break;
		}
		fprint(2, "\tnext = %p\n", cur->next);
		fprint(2, "}\n");
	}
}

void
dumpall(void)
{
	dumpsrc();
	dumplabels();
	dumpdata();
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
	assert(slen > 0);
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

	for(i = 0; i < nelem(insts); i++){
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
	return strtoul(str, nil, 16);
}

void
addlabel(char *name, u32int addr)
{
	E("addlabel");
	Label *cur;

	cur = labels;
	if(cur != nil){
		while(cur->next != nil)
			cur = cur->next;
		cur->next = emalloc(sizeof(Label));
		cur = cur->next;
	} else {
		labels = emalloc(sizeof(Label));
		cur = labels;
	}
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
		DEBUG(smprint("comp: adding inst %s (cur = %p, inst = %p)", inst->inst, cur, inst));
	} else {
		DEBUG("initializing src");
		src = inst;
		DEBUG(smprint("comp: adding inst %s (%p)", inst->inst, src));
	}
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

	DEBUG(smprint("preproc: %s", str));
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
		adddata(strtoul(toks[1], nil, 16));
		return;
	}
	if(strcmp(".vdata", toks[0]) == 0){
		for(i = 1; i < toksl; i++)
			adddata(strtoul(toks[i], nil, 16));
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
	if(str[0] == '\0')
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
	DEBUG(smprint("encoding: %s (len = %ud, slen = %ud)", inst->inst, inst->len, inst->slen));
	DEBUG(smprint("encoding: raw = %s", inst->raw));
	einst->otype = 0;
	einst->type = def->type;
	einst->len = def->len;
	einst->inst = def->op;
	for(i = 0; i < (inst->slen-1); i++){
		if(strlen(inst->args[i]) == 2){
			einst->args[iaddr] = getreg(inst->args[i]);
			iaddr++;
		} else {
			encode32int(decodeaddr(inst->args[i]), &einst->args[iaddr]);
			iaddr += sizeof(u32int);
		}
	}
	einst->src = inst;
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
	einst->datsrc = d;
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
	curaddr = 0;
	for(;;){
		if(curdat != nil && curdat->addr == curaddr){
			DEBUG(smprint("encoding data: addr = %ux, dat = %ux", curdat->addr, curdat->dat));
			curout->next = encodedata(curdat);
			curaddr += sizeof(u32int);
			curdat = curdat->next;
		} else if(curinst != nil){
			DEBUG(smprint("encoding inst: addr = %ux, inst = %s", curaddr, curinst->inst));
			curout->next = encodeinst(curinst);
			curout->next->addr = curaddr;
			curaddr += curinst->len;
			curinst = curinst->next;
		} else
			break;
		curout = curout->next;
	}
	output = start->next;
}

int
dumpoutput(int bf)
{
	E("dumpoutput");
	Instd *cur;
	u8int tmp;

	if(output == nil)
		panic("no output generated");
	for(cur = output; cur != nil; cur = cur->next){
		switch(cur->otype){
		case 0:
			DEBUG(smprint("writing at %x instruction '%s'", cur->addr, cur->src->raw));
			tmp = cur->type + cur->len;
			DEBUG(smprint("tmp = %x", tmp));
			if(write(bf, &tmp, 1) < 1)
				goto fail;
			if(write(bf, &cur->inst, 1) < 1)
				goto fail;
			if(cur->len - 2 > 0)
				if(write(bf, &cur->args[0], (cur->len - 2)) < (cur->len - 2))
					goto fail;
			break;
		case 1:
			DEBUG(smprint("writing data at %x", cur->addr));
			if(write(bf, &cur->dat[0], 4) < 4)
				goto fail;
			break;
		default:
			panic("unknown output list item");
		}
	}
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
	Biobuf *srcfd;
	int outfd;
	int st;

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
	outfd = create(outfile, OWRITE|OTRUNC, 0660);
	if(!srcfd || outfd < 0)
		panic("could not open src or output files");
	
	for(;;){
		st = readsrcline(srcfd);
		if(st == -2)
			break;
		if(st == -1)
			panic("general read error");
	}
	if(debug) dumpall();
	generateoutput();
	if(debug) dumpgens();
	if(dumpoutput(outfd) != 0)
		panic("could not dump the assembled file");
	Bterm(srcfd);
	close(outfd);
	exits(nil);
}
