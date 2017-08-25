#include <u.h>
#include <libc.h>
#include <thread.h>
#include <bio.h>
#include "cpu.h"

void
dumpregs(Cpu *c)
{
	int i, j;
	Vect *vreg;
	u32int *sreg;

	for(i = 0; i < SY+1; i++){
		if((i >= R0) && (i <= S7)){
			sreg = getregister(c, i);
			print("%s = %ux\n", regnames[i], *sreg);
		} else if((i >= V0) && (i <= V7)) {
			vreg = getvreg(c, i);
			print("%s = { ", regnames[i]);
			for(j = 0; j < VECLEN; j++)
				print("%ux ", vreg->dat[j]);
			print("}\n");
		} else if (i == OP || i == PC || i == SP || i == FL || i == SY){
			sreg = getregister(c, i);
			print("%s = %ux\n", regnames[i], *sreg);
		}
	}
}
