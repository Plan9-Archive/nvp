#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>
#include "cpu.h"
#include "devs.h"

Biobuf *stdin;

void
consinit(void)
{
	stdin = Bfdopen(0, OREAD);
	return;
}

u8int
consread(void)
{
	return (u8int)Bgetc(stdin);
}

void
conswrite(u8int dat)
{
	write(1, &dat, 1);
}

Device console = {
	.active = 1,
	.init = consinit,
	.read = consread,
	.write = conswrite,
	.status = nil,
	.cmd = nil,
};
