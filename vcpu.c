// single machine vector cpu
// loads and stores are not parallel with this config
#include <u.h>
#include <libc.h>
#include <thread.h>
#include "cpu.h"

Channel *cpool[VECLEN];
int p;
QLock il;

void
readvect(Cpu *c, Vect *v, u32int addr)
{
	for(int i = 0; i < VECLEN; i++)
		readreg(c, &v->dat[i], addr+(i*4));
}

void
writevect(Cpu *c, Vect *v, u32int addr)
{
	for(int i = 0; i < VECLEN; i++)
		writereg(c, &v->dat[i], addr+(i*4));
}

void
vectworker(void *arg)
{
	int tmp = 1;
	Vinst *v;
	Cpu *c;

	c = arg;
	qlock(&il);
	threadsetname("nvp thread worker %d", p);
	p++;
	qunlock(&il);
	for(;;){
		v = recvp(c->vunit);
//		dprint("vectworker entry");
		if(v == nil)
			panic(smprint("vector unit failure: %r"));
//		dprint(smprint("v->inst = %x, v->reply = %p", v->inst, v->reply));
		switch(v->inst){
		case VECEQ:
			tmp = *v->src1 == *v->src2 ? 1 : 0;
			send(v->reply, &tmp);
			break;
		case VECGT:
			tmp = *v->src1 > *v->src2 ? 1 : 0;
			send(v->reply, &tmp);
			break;
		case VECLT:
			tmp = *v->src1 < *v->src2 ? 1 : 0;
			send(v->reply, &tmp);
			break;
		case VECADD:
			*v->dest = *v->src1 + *v->src2;
			send(v->reply, &tmp);
			break;
		case VECSUB:
			*v->dest = *v->src1 - *v->src2;
			send(v->reply, &tmp);
			break;
		case VECMUL:
			*v->dest = *v->src1 * *v->src2;
			send(v->reply, &tmp);
			break;
		case VECDIV:
			if(*v->src2 == 0)
				tmp = -1;
			else
				*v->dest = *v->src1 / *v->src2;
			send(v->reply, &tmp);
			break;
		case VECMOD:
			if(*v->src2 == 0)
				tmp = -1;
			else
				*v->dest = *v->src1 / *v->src2;
			send(v->reply, &tmp);
			break;
		default:
			panic("unknown vector instruction");
			break;
		}
		tmp = 1;
	}
}

void
startvectworker(Cpu *c)
{
	Channel *ch;
	int i;
	void *arg;
	
	qlock(c);
	ch = chancreate(sizeof(Vinst*), VECLEN);
	if(ch == nil){
		panic("chancreate failed");
	}
	c->vunit = ch;
	arg = c;
	for(i = 0; i < VECLEN; i++){
		cpool[i] = chancreate(sizeof(u32int), 0);
		if(cpool[i] == nil)
			panic("chancreate failed");
	}
	p = 1;
	for(i = 0; i < VECLEN; i++)
		proccreate(vectworker, arg, mainstacksize);
	qunlock(c);
}

Vect*
getvreg(Cpu *c, u32int regn)
{
	Regr r;
	Vect *rval;

	r = _getregister(c, regn);
	rval = r.type == 1 ? r.v : nil;
	return rval;
}

void
sendvectop(Cpu *c, Inst *inst)
{
	switch(inst->type){
	case 0x1:
//		dprint("sendvectmemop call");
		sendvectmemop(c, inst);
		break;
	case 0x4:
//		dprint("sendvectmathop call");
		sendvectmathop(c, inst);
		break;
	}
}

void
sendvectmathop(Cpu *c, Inst *inst)
{
	Vinst v[VECLEN];
	Vect *src1 = nil, *src2 = nil, *dest = nil;
	u32int *fl;
	int i, ins = 0;
	int rval, tmp;

	fl = getregister(c, FL);
	switch(inst->op){
	case 0x10: // eqv
		ins = VECEQ;
		src1 = getvreg(c, inst->args[0]);
		src2 = getvreg(c, inst->args[1]);
		break;
	case 0x11: // gtv
		ins = VECGT;
		src1 = getvreg(c, inst->args[0]);
		src2 = getvreg(c, inst->args[1]);
		break;
	case 0x12: // ltv
		ins = VECLT;
		src1 = getvreg(c, inst->args[0]);
		src2 = getvreg(c, inst->args[1]);
		break;
	case 0x13: // addv
		ins = VECADD;
		dest = getvreg(c, inst->args[0]);
		src1 = getvreg(c, inst->args[1]);
		src2 = getvreg(c, inst->args[2]);
		break;
	case 0x14: // subv
		ins = VECSUB;
		dest = getvreg(c, inst->args[0]);
		src1 = getvreg(c, inst->args[1]);
		src2 = getvreg(c, inst->args[2]);
		break;
	case 0x15: // mulv
		ins = VECMUL;
		dest = getvreg(c, inst->args[0]);
		src1 = getvreg(c, inst->args[1]);
		src2 = getvreg(c, inst->args[2]);
		break;
	case 0x16: // divv
		ins = VECDIV;
		dest = getvreg(c, inst->args[0]);
		src1 = getvreg(c, inst->args[1]);
		src2 = getvreg(c, inst->args[2]);
		break;
	case 0x17: // modv
		ins = VECMOD;
		dest = getvreg(c, inst->args[0]);
		src1 = getvreg(c, inst->args[1]);
		src2 = getvreg(c, inst->args[2]);
		break;
	}
	for(i = 0; i < VECLEN; i++){
		v[i].c = c;
		v[i].inst = ins;
		v[i].dest = &dest->dat[i];
		v[i].src1 = &src1->dat[i];
		v[i].src2 = &src2->dat[i];
		v[i].reply = cpool[i];
		sendp(c->vunit, &v[i]);
	}
	rval = 0;
	for(i = 0; i < VECLEN; i++){
		recv(v[i].reply, &tmp);
		rval += tmp;
	}
	switch(ins){
	case VECEQ:
	case VECGT:
	case VECLT:
		*fl = rval == VECLEN ? 1 : 0;
		break;
	default:
		if(rval < 0)
			fprint(2, "nvp: vector unit: divide by zero\n");
		if(rval < 0 && c->cont == 0)
			panic("vector unit: divide by zero");
		break;
	}
}

void
sendvectmemop(Cpu *c, Inst *inst)
{
	Vect *vdest, *vsrc;
	u32int *rsrc, *rsrc2, *rdest, *fl;
	u32int addr;
	int i;

	fl = getregister(c, FL);
	switch(inst->op){
	case 0x10: // loadv
		vdest = getvreg(c, inst->args[0]);
		addr = inst->args[1];
		readvect(c, vdest, addr);
		break;
	case 0x11: // storev
		vsrc = getvreg(c, inst->args[0]);
		addr = inst->args[1];
		writevect(c, vsrc, addr);
		break;
	case 0x12: // movev
		vsrc = getvreg(c, inst->args[0]);
		vdest = getvreg(c, inst->args[1]);
		memcpy(vdest->dat, vsrc->dat, VECLEN);
		break;
	case 0x13: // rloadv
		vdest = getvreg(c, inst->args[0]);
		rsrc = getregister(c, inst->args[1]);
		readvect(c, vdest, *rsrc);
		break;
	case 0x14: // rstorev
		vsrc = getvreg(c, inst->args[0]);
		rdest = getregister(c, inst->args[1]);
		writevect(c, vsrc, *rdest);
		break;
	case 0x15: // rloadsv
		rdest = getregister(c, inst->args[0]);
		vsrc = getvreg(c, inst->args[1]);
		rsrc = getregister(c, inst->args[2]);
		if(*rsrc >= VECLEN)
			*fl = 1;
		else
			*rdest = vsrc->dat[*rsrc];
		break;
	case 0x16: // rstoresv
		vdest = getvreg(c, inst->args[0]);
		rsrc = getregister(c, inst->args[1]);
		rsrc2 = getregister(c, inst->args[2]);
		if(*rsrc2 >= VECLEN)
			*fl = 1;
		else
			vdest->dat[*rsrc2] = *rsrc;
		break;
	case 0x17: // clrv
		vdest = getvreg(c, inst->args[0]);
		for(i = 0; i < VECLEN; i++)
			vdest->dat[i] = 0;
		break;
	}
}
