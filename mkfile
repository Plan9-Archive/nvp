</$objtype/mkfile

BIN=/$objtype/bin
TARG=nvp
ACIDTARG=nvp.acid
OFILES=cpu.$O vcpu.$O mem.$O main.$O
HFILES=cpu.h
ACIDFILES=cpu.acid vcpu.acid mem.acid main.acid
CLEANFILES=$ACIDFILES nvp.acid

</sys/src/cmd/mkone

allacid: $ACIDFILES
	cat $ACIDFILES > $ACIDTARG
