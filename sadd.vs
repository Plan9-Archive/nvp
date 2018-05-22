; scalar version of add.vs
	loadc sp *stack
	jmp *start

.label vect1
.vdata ffff 5f5f 4567 234 8098 2356a a4b5c6 aaaa
.label vect2
.vdata aaaa bbbb cccc dddd eeee ffff 1111 2222
.label numloops
.data 64

.label svadd
	loadc r0 0
	loadc r1 1
	loadc r2 8
.label svaddloop
	eqs r0 r2
	cjmp *svadddone
	rloadsv r3 v0 r0
	rloadsv r4 v1 r0
	adds r5 r3 r4
	rstoresv v2 r5 r0
	adds r0 r0 r1
	jmp *svaddloop
.label svadddone
	clrf
	bp
	return

.label start
	loadv v0 *vect1
	loadv v1 *vect2
	loads s0 *numloops
.label loop
	clrf
	loadc r0 0
	eqs s0 r0
	cjmp *done
	bp
	call *svadd
	loadc r0 1
	subs s0 s0 r0
	jmp *loop
.label done
	bp
	halt
; throw 64 bytes of nothing in case things are buggered
.vdata 0 0 0 0 0 0 0 0
.vdata 0 0 0 0 0 0 0 0
.label stack
.eof
