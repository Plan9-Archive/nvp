</$objtype/mkfile

BIN=/$objtype/bin
TARG=$O.nvp $O.vpasm
OFILES=cpu.$O vcpu.$O mem.$O main.$O vpasm.$0 debug.$O debugger.$O console.$O disk.$O
HFILES=cpu.h regs.h inst.h vpasm.h devs.h
ACIDFILES=cpu.acid vcpu.acid mem.acid main.acid debug.acid debugger.acid console.acid disk.acid

%.h:

%.$O:	%.c
	$CC $CFLAGS $stem.c

%.$O:	%.s
	$AS $AFLAGS $stem.s

%.acid: %.c
	$CC -a $stem.c > $stem.acid

%.vx: $O.vpasm
	./$O.vpasm -s $stem.vs -o $stem.vx

all:V: $TARG test.vx

nuke:V:
	rm -f *.[$OS] [$OS].out y.tab.? lex.yy.c y.debug y.output *.vx $TARG $CLEANFILES vpasm.acid $ACIDFILES

clean:V:
	rm -f *.[$OS] [$OS].out y.tab.? lex.yy.c y.debug y.output *.vx [$OS].nvp [$OS].vpasm $TARG $CLEANFILES vpasm.acid $ACIDFILES

$O.nvp: cpu.$O vcpu.$O mem.$O main.$O debug.$O debugger.$O console.$O disk.$O cpu.h regs.h inst.h vpasm.h
	$LD $LDFLAGS -o $O.nvp cpu.$O vcpu.$O mem.$O main.$O debug.$O debugger.$O console.$O disk.$O

$O.vpasm: vpasm.$O regs.h inst.h vpasm.h
	$LD $LDFLAGS -o $O.vpasm vpasm.$O

runtestdl:V: $O.vpasm $O.nvp test.vx
	./$O.nvp -d -f test.vx -i 50

runtest:V: $O.vpasm $O.nvp test.vx
	./$O.nvp -f test.vx

vpasm:V: $O.vpasm

nvp:V: $O.nvp

nvp.acid:V: $ACIDFILES
	cat $ACIDFILES > nvp.acid

asmtest:V: $O.vpasm
	./$O.vpasm -d -s test.vs -o test.vx
