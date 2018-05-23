; vector version of sadd.vs
	loadc sp *stack
	jmp *start

.label vect1
.vdata ffff 5f5f 4567 234 8098 2356a a4b5c6 aaaa
.label vect2
.vdata aaaa bbbb cccc dddd eeee ffff 1111 2222
.label numloops
.data 64

.label svadd
	addv v2 v0 v1
	return

.label start
	loadv v0 *vect1
	loadv v1 *vect2
	loads s0 *numloops
.label loop
	clrf
	loadc r0 0
	eqs s0 r0
	cjmp *doneloop
	call *svadd
	loadc r0 1
	subs s0 s0 r0
	jmp *loop
.label done
	bp
	halt
.label doneloop
	bp
.label dloop
	jmp *dloop
; throw 64 bytes of nothing in case things are buggered
.vdata 0 0 0 0 0 0 0 0
.vdata 0 0 0 0 0 0 0 0
.label stack
.eof
