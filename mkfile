</$objtype/mkfile

BIN=/$objtype/bin
TARG=$O.nvp $O.vpasm
OFILES=cpu.$O vcpu.$O mem.$O main.$O vpasm.$0
HFILES=cpu.h reg.h inst.h vpasm.h
ACIDFILES=cpu.acid vcpu.acid mem.acid main.acid vpasm.acid
CLEANFILES=$ACIDFILES nvp.acid vpasm.acid

# %.$O:	$HFILES

%.$O:	%.c
	$CC $CFLAGS $stem.c

%.$O:	%.s
	$AS $AFLAGS $stem.s

%.acid: %.$O $HFILES
	$CC $CFLAGS -a $stem.c >$target

%.vx: $O.vpasm
	./$O.vpasm -d -s $stem.vs -o $stem.vx

nuke:V:
	rm -f *.[$OS] [$OS].out y.tab.? lex.yy.c y.debug y.output *.acid $TARG $CLEANFILES

clean:V:
	rm -f *.[$OS] [$OS].out y.tab.? lex.yy.c y.debug y.output $TARG $CLEANFILES

$O.nvp: cpu.$O vcpu.$O mem.$O main.$O
	$LD $LDFLAGS -o $O.nvp $prereq

$O.vpasm: vpasm.$O
	$LD $LDFLAGS -o $O.vpasm vpasm.$O

allacid: $ACIDFILES
	cat cpu.acid vcpu.acid mem.acid main.acid > nvp.acid

vpasm:V: $O.vpasm

nvp:V: $O.nvp

all:V: $TARG

asmtest:V: $O.vpasm test.vx
