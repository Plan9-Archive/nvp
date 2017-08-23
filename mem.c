#include <u.h>
#include <libc.h>
#include <thread.h>
#include "cpu.h"

u8int *memory;
u32int memlen;
u32int lastacc;

void
meminit(u32int len)
{
	memory = mallocz(sizeof(u8int)*len, 1);
	memlen = len;
	if(memory)
		return;
	perror("main memory alloc failed");
	threadexitsall("main memory alloc failed");
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
	lastacc = addr;
	if(addr >= memlen)
		memaccess_fail();
	return memory[addr];
}

void
memwrite(u8int *b, u32int addr)
{
	lastacc = addr;
	if(addr >= memlen)
		memaccess_fail();
	memory[addr] = *b;
}
