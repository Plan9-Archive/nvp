; nvp test. this will do a loop of *many* vector instructions to allow testing of speed
	jmp *start

; some vectors to munch on
.label vect1
.vdata ffff 5f5f 4567 234 8098 2356a a4b5c6 aaaa
.label vect2
.vdata aaaa bbbb cccc dddd eeee ffff 1111 2222

; do 50000 of these operations
.label numloops
.data c350

.rstart
.label start
	loadc op *RSTART
	loadv v0 *vect1
	loadv v1 *vect2
	loads s0 *numloops
.label loop
	clrf
	loadc r0 0
	eq s0 r0
	crjmp *done
	addv v2 v0 v1
	subv v3 v0 v1
	mulv v4 v0 v1
	divv v5 v0 v1
	bp
	loadc r0 1
	subs s0 s0 r0
	rjmp *loop
.label *done
	halt
.rend
