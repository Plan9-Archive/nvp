#include <u.h>
#include <libc.h>
#include <thread.h>
#include "cpu.h"

Regr
_getregister(Cpu *c, u32int regn)
{
	Regr r;
	u8int tmp;

	switch(regn){
	case R0:
	case R1:
	case R2:
	case R3:
	case R4:
	case R5:
	case R6:
	case R7:
	case S0:
	case S1:
	case S2:
	case S3:
	case S4:
	case S5:
	case S6:
	case S7:
		tmp = regn - 0x10;
		r.type = 0;
		r.s = &c->g_regs[tmp];
		break;
	case V0:
	case V1:
	case V2:
	case V3:
	case V4:
	case V5:
	case V6:
	case V7:
		tmp = regn - 0x20;
		r.type = 1;
		r.v = &c->v_regs[tmp];
		break;
	case OP:
	case PC:
	case SP:
	case FL:
	case SY:
		tmp = regn - 0x30;
		r.type = 2;
		r.s = &c->c_regs[tmp];
		break;
	}
	return r;
}

u32int*
getregister(Cpu *c, u32int regn)
{
	Regr r;
	u32int *rval = nil;

	r = _getregister(c, regn);
	if(r.type == 0 || r.type == 2)
		rval = r.s;
	return rval;
}

void
execute(Cpu *c, Inst *inst)
{
	switch(inst->type){
	case 0xf:
		doimplop(c, inst);
		break;
	case 0x1:
	case 0x4:
//		dprint("dovectorop call");
		sendvectop(c, inst);
		break;
	case 0x0:
//		dprint("doscalarmemop call");
		doscalarmemop(c, inst);
		break;
	case 0x2:
//		dprint("docontrolop call");
		docontrolop(c, inst);
		break;
	case 0x3:
//		dprint("doscalarmathop call");
		doscalarmathop(c, inst);
		break;
	}
}

void
readreg(Cpu *c, u32int *r, u32int addr)
{
	u8int b[4];

	for(int i = 0; i < 4; i++)
		b[i] = c->memread(addr+i);
	*r = b[0]<<24 | b[1]<<16 | b[2]<<8 | b[3];
}

void
writereg(Cpu *c, u32int *r, u32int addr)
{
	u8int b[4];

	b[0] = *r>>24 & 0xff;
	b[1] = *r>>16 & 0xff;
	b[2] = *r>>8 & 0xff;
	b[3] = *r & 0xff;
	for(int i = 0; i < 4; i++)
		c->memwrite(&b[i], addr+i);
}

void
doscalarmemop(Cpu *c, Inst *inst)
{
	u32int *reg1, *reg2;
	u32int addr;

	switch(inst->op){
	case 0x10: // loads
		reg1 = getregister(c, inst->args[0]);
		addr = inst->args[1];
		readreg(c, reg1, addr);
		break;
	case 0x11: // stores
		reg1 = getregister(c, inst->args[0]);
		addr = inst->args[1];
		writereg(c, reg1, addr);
		break;
	case 0x12: // moves
		reg1 = getregister(c, inst->args[0]);
		reg2 = getregister(c, inst->args[1]);
		*reg2 = *reg1;
		break;
	case 0x13: // rloads
		reg1 = getregister(c, inst->args[0]);
		reg2 = getregister(c, inst->args[1]);
		readreg(c, reg1, *reg2);
		break;
	case 0x14: // rstores
		reg1 = getregister(c, inst->args[0]);
		reg2 = getregister(c, inst->args[1]);
		writereg(c, reg1, *reg2);
		break;
	case 0x15: // push
		reg1 = getregister(c, inst->args[0]);
		reg2 = getregister(c, SP);
		writereg(c, reg1, *reg2);
		(*reg2)++;
		break;
	case 0x16: // pop
		reg1 = getregister(c, inst->args[0]);
		reg2 = getregister(c, SP);
		(*reg2)--;
		readreg(c, reg1, *reg2);
		break;
	case 0x17: // loadc
		reg1 = getregister(c, inst->args[0]);
		*reg1 = inst->args[1];
		break;
	case 0x18: // clrs
		reg1 = getregister(c, inst->args[0]);
		*reg1 = 0;
		break;
	}
}

void
docontrolop(Cpu *c, Inst *inst)
{
	u32int *reg1;
	u32int addr;
	u32int *sp, *sy, *fl, (*pc), *op;

	sp = getregister(c, SP);
	sy = getregister(c, SY);
	fl = getregister(c, FL);
	pc = getregister(c, PC);
	op = getregister(c, OP);
	switch(inst->op){
	case 0x10: // jmp
		(*pc) = inst->args[0];
//		dprint(smprint("jmp: PC = %x, inst->args[0] = %x", *pc, inst->args[0]));
		break;
	case 0x11: // rjmp
		(*pc) = *op+inst->args[0];
//		dprint(smprint("jmp: PC = %x, inst->args[0] = %x", *pc, inst->args[0]));
		break;
	case 0x12: // jmpr
		reg1 = getregister(c, inst->args[0]);
		(*pc) = *reg1;
//		dprint(smprint("jmp: PC = %x, inst->args[0] = %x", *pc, inst->args[0]));
		break;
	case 0x13: // rjmpr
		reg1 = getregister(c, inst->args[0]);
		(*pc) = *op+*reg1;
//		dprint(smprint("jmp: PC = %x, inst->args[0] = %x", *pc, inst->args[0]));
		break;
	case 0x14: // clrf
		*fl = 0;
		break;
	case 0x15: // cjmp
		addr = inst->args[0];
		if(*fl > 0){
			*fl = 0;
			(*pc) = addr;
		}
//		dprint(smprint("jmp: PC = %x, inst->args[0] = %x", *pc, inst->args[0]));
		break;
	case 0x16: // crjmp
		addr = inst->args[0]+*op;
		if(*fl > 0){
			*fl = 0;
			(*pc) = addr;
		}
//		dprint(smprint("jmp: PC = %x, inst->args[0] = %x", *pc, inst->args[0]));
		break;
	case 0x17: // cjmpr
		reg1 = getregister(c, inst->args[0]);
		addr = *reg1;
		if(*fl > 0){
			*fl = 0;
			(*pc) = addr;
		}
//		dprint(smprint("jmp: PC = %x, inst->args[0] = %x", *pc, inst->args[0]));
		break;
	case 0x18: // crjmpr
		reg1 = getregister(c, inst->args[0]);
		addr = *reg1+*op;
		if(*fl > 0){
			*fl = 0;
			(*pc) = addr;
		}
//		dprint(smprint("jmp: PC = %x, inst->args[0] = %x", *pc, inst->args[0]));
		break;
	case 0x19: // setsys
		addr = inst->args[0];
		*sy = addr;
//		dprint(smprint("jmp: PC = %x, inst->args[0] = %x", *pc, inst->args[0]));
		break;
	case 0x1a: // syscall
		addr = *sy;
		writereg(c, pc, *sp);
		(*sp)++;
		(*pc) = *sy;
		break;
	case 0x1b: // call
		addr = inst->args[0];
		writereg(c, pc, *sp);
		(*sp)++;
		(*pc) = addr;
		break;
	case 0x1c: // rcall
		addr = inst->args[0]+*op;
		writereg(c, pc, *sp);
		(*sp)++;
		(*pc) = addr;
		break;
	case 0x1d: // return
		readreg(c, &addr, *sp);
		(*sp)--;
		(*pc) = addr;
		break;
	case 0x1f: // halt
		cpuhalt();
		break;
	}
}

void
doscalarmathop(Cpu *c, Inst *inst)
{
	u32int *reg1, *reg2, *reg3;
	u32int *fl;

	fl = getregister(c, FL);
	switch(inst->op){
	case 0x10: // eqs
		reg1 = getregister(c, inst->args[0]);
		reg2 = getregister(c, inst->args[1]);
		*fl = *reg1 == *reg2 ? 1 : 0;
		break;
	case 0x11: // gts
		reg1 = getregister(c, inst->args[0]);
		reg2 = getregister(c, inst->args[1]);
		*fl = *reg1 > *reg2 ? 1 : 0;
		break;
	case 0x12: // lts
		reg1 = getregister(c, inst->args[0]);
		reg2 = getregister(c, inst->args[1]);
		*fl = *reg1 < *reg2 ? 1 : 0;
		break;
	case 0x13: // adds
		reg1 = getregister(c, inst->args[0]);
		reg2 = getregister(c, inst->args[1]);
		reg3 = getregister(c, inst->args[2]);
		*reg1 = *reg2 + *reg3;
		break;
	case 0x14: // subs
		reg1 = getregister(c, inst->args[0]);
		reg2 = getregister(c, inst->args[1]);
		reg3 = getregister(c, inst->args[2]);
		*reg1 = *reg2 - *reg3;
		break;
	case 0x15: // muls
		reg1 = getregister(c, inst->args[0]);
		reg2 = getregister(c, inst->args[1]);
		reg3 = getregister(c, inst->args[2]);
		*reg1 = *reg2 * *reg3;
		break;
	case 0x16: // divs
		reg1 = getregister(c, inst->args[0]);
		reg2 = getregister(c, inst->args[1]);
		reg3 = getregister(c, inst->args[2]);
		if(*reg3 != 0)
			*reg1 = *reg2 / *reg3;
		else
			*fl = 1;
		break;
	case 0x17: // mods
		reg1 = getregister(c, inst->args[0]);
		reg2 = getregister(c, inst->args[1]);
		reg3 = getregister(c, inst->args[2]);
		if(*reg3 != 0)
			*reg1 = *reg2 % *reg3;
		else
			*fl = 1;
		break;
	case 0x18: // nands
		reg1 = getregister(c, inst->args[0]);
		reg2 = getregister(c, inst->args[1]);
		reg3 = getregister(c, inst->args[2]);
		*reg1 = ~(*reg2 & *reg3);
		break;
	}
}

void
fetchdecode(Cpu *c, Inst *inst)
{
	u32int *pc;
	u8int t;
	u8int len;
	u8int i;
	u8int b[16];
	
	pc = getregister(c, PC);
	// fetch the entire instruction
//	dprint(smprint("PC = %x", *pc));
	b[0] = c->memread((*pc)++);
//	dprint(smprint("PC (first fetch) = %x", *pc));
	b[1] = c->memread((*pc)++);
//	dprint(smprint("PC (second fetch) = %x", *pc));
//	dprint(smprint("b[0] = %x, b[1] = %x", b[0], b[1]));
	len = (b[0] & 0x0f)-2;
	t = b[0] & 0xf0;
	inst->type = t >> 4;
	inst->op = b[1];
//	dprint(smprint("inst->type = %x, len = %x, inst->op = %x", 
//			inst->type, len, inst->op));
	for(i = 0; i < len; i++)
		b[i] = c->memread((*pc)++);
//	dprint("instruction decode");
	switch(inst->type){
	case 0xf:
		switch(inst->op){
		case 0x11:
			inst->args[0] = b[0] << 24 | b[1] << 16 | b[2] << 8 | b[3];
			break;
		case 0x12:
			inst->args[0] = b[0];
			break;
		}
		break;
	case 0x0:
//		dprint("scalar memory instruction");
		inst->args[0] = b[0];
		switch(inst->op){
		case 0x10:
		case 0x11:
		case 0x17:
			inst->args[1] = b[1] << 24 | b[2] << 16 | b[3] << 8 | b[4];
			break;
		case 0x12:
		case 0x13:
		case 0x14:
			inst->args[1] = b[1];
			break;
		}
		break;
	case 0x1:
//		dprint("vector memory instruction");
		inst->args[0] = b[0];
		switch(inst->op){
		case 0x10:
		case 0x11:
			inst->args[1] = b[1] << 24 | b[2] << 16 | b[3] << 8 | b[4];
			break;
		case 0x12:
		case 0x13:
		case 0x14:
			inst->args[1] = b[1];
			break;
		case 0x15:
		case 0x16:
			inst->args[1] = b[1];
			inst->args[2] = b[2];
			break;
		}
		break;
	case 0x2:
//		dprint("jmp instruction");
		switch(inst->op){
		case 0x10:
		case 0x11:
		case 0x15:
		case 0x16:
		case 0x19:
		case 0x1b:
		case 0x1c:
			inst->args[0] = b[0] << 24 | b[1] << 16 | b[2] << 8 | b[3];
//			dprint(smprint("inst->args[0] = %x", inst->args[0]));
			break;
		case 0x12:
		case 0x13:
		case 0x17:
		case 0x18:
			inst->args[0] = b[0];
			break;
		}
		break;
	case 0x3:
	case 0x4:
		inst->args[0] = b[0];
		inst->args[1] = b[1];
		if(inst->op >= 0x13)
			inst->args[2] = b[2];
		break;
	default:
		dprint(smprint("inst->type = %x, inst->op = %x", inst->type, inst->op));
		panic("unknown instruction");
		break;
	}
}

void
startexec(Cpu *c, u32int startaddr)
{
	u32int (*pc);
	Inst *inst;

	pc = getregister(c, PC);
	(*pc) = startaddr;
	inst = malloc(sizeof(Inst));
	if(!inst)
		panic("failed malloc");
	for(;;){
		if(c->icount > c->ilimit && c->ilimit > 0){
			fprint(2, "%s: cycle limit reached\n", argv0);
			cpuhalt();
		}
		if(c->pause == 1){
			sleep(10);
			continue;
		}
		fetchdecode(c, inst);
		execute(c, inst);
		c->icount++;
	}
}

Cpu*
makecpu(void)
{
	Cpu *c;

	c = mallocz(sizeof(Cpu), 1);
	if(c == nil)
		panic("bad malloc");
	c->memread = memread;
	c->memwrite = memwrite;
	c->ilimit = 0;
	c->icount = 0;
	return c;
}

void
cpuhalt(void)
{
	fprint(2, "%s: cpu halted\n", argv0);
	threadexitsall(nil);
}
