#include <u.h>
#include <libc.h>
#include <thread.h>
#include "cpu.h"
#include "devs.h"

enum {
	DEVSTART = 0xffffff00,
};

u8int *memory;
u32int memlen;
u32int lastacc;
Device *devs[16];

void
meminit(u32int len)
{
	memory = mallocz(sizeof(u8int)*len, 1);
	memlen = len;
	if(!memory)
		panic("main memory alloc failed");
	devs[0] = &console;
	devs[1] = &disk;
}

void
memaccess_fail(void)
{
	fprint(2, "PC = %x, lastacc = %x (max = %x)\n", cpu0->c_regs[PC], lastacc, memlen);
	panic("bad memory access");
}

u8int
memread(u32int addr)
{
	u8int devn;

	lastacc = addr;
	if(addr >= DEVSTART){
		devn = (addr & 0xf0) >> 4;
		return devread(devs[devn], (addr & 0x0f));
	} else if(addr >= memlen)
		memaccess_fail();
	return memory[addr];
}

void
memwrite(u8int *b, u32int addr)
{
	u8int devn;

	lastacc = addr;
	if(addr >= DEVSTART){
		devn = (addr & 0xf0) >> 4;
		devwrite(devs[devn], (addr & 0x0f), *b);
		return;
	}
	if(addr >= memlen)
		memaccess_fail();
	memory[addr] = *b;
}

u8int
devread(Device *d, u8int addr)
{
	if(d == nil)
		panic("unknown device");
	switch(addr){
	case 0x0:
		if(d->read != nil)
			return d->read();
		break;
	case 0x1:
		if(d->status != nil)
			return d->status();
		break;
	default:
		return d->dat[addr-2];
		break;
	}
	return 0;
}

void
devwrite(Device *d, u8int addr, u8int dat)
{
	if(d == nil)
		panic("unknown device");
	switch(addr){
	case 0x0:
		if(d->write != nil)
			d->write(dat);
		break;
	case 0x1:
		if(d->cmd != nil)
			d->cmd(dat);
		break;
	default:
		d->dat[addr-2] = dat;
		break;
	}
}
