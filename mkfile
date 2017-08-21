</$objtype/mkfile

BIN=/$objtype/bin
TARG=$O.nvp $O.vpasm
OFILES=cpu.$O vcpu.$O mem.$O main.$O vpasm.$0
HFILES=cpu.h regs.h inst.h vpasm.h
ACIDFILES=cpu.acid vcpu.acid mem.acid main.acid

%.h:

%.$O:	%.c
	$CC $CFLAGS $stem.c

%.$O:	%.s
	$AS $AFLAGS $stem.s

%.vx: $O.vpasm
	./$O.vpasm -d -s $stem.vs -o $stem.vx

all:V: $TARG

nuke:V:
	rm -f *.[$OS] [$OS].out y.tab.? lex.yy.c y.debug y.output *.vx $TARG $CLEANFILES

clean:V:
	rm -f *.[$OS] [$OS].out y.tab.? lex.yy.c y.debug y.output *.vx [$OS].nvp [$OS].vpasm $TARG $CLEANFILES

$O.nvp: cpu.$O vcpu.$O mem.$O main.$O
	$LD $LDFLAGS -o $O.nvp $prereq

$O.vpasm: vpasm.$O regs.h inst.h vpasm.h
	$LD $LDFLAGS -o $O.vpasm vpasm.$O

vpasm:V: $O.vpasm

nvp:V: $O.nvp

asmtest:V: $O.vpasm test.vx
