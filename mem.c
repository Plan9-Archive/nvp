#include <u.h>
#include <libc.h>
#include <thread.h>
#include "cpu.h"

u8int *memory;
u32int memlen;

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
	perror("bad memory access");
	threadexitsall("bad memory access");
}

u8int
memread(u32int addr)
{
	if(addr >= memlen)
		memaccess_fail();
	return memory[addr];
}

void
memwrite(u8int *b, u32int addr)
{
	if(addr >= memlen)
		memaccess_fail();
	memory[addr] = *b;
}
