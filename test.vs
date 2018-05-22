; nvp test. this will do a loop of *many* vector instructions to allow testing of speed
	jmp *start

; some vectors to munch on
.label vect1
.vdata ffff 5f5f 4567 234 8098 2356a a4b5c6 aaaa
.label vect2
.vdata aaaa bbbb cccc dddd eeee ffff 1111 2222
.label minvect
.vdata abab 0 0 3333 0 0 0 0
.label vstore1
.vdata 0 0 0 0 0 0 0 0
.label vstore2
.vdata 0 0 0 0 0 0 0 0

.label numloops
; do 50000 of these operations
;.data c350
; do 100 of these operations
.data 64

.label fillvect
	clrs s0
	loadc s1 1
	loadc s2 8
.label fillvect_loop
	rstoresv v0 r1 s0
	adds s0 s0 s1
	lts s0 s2
	cjmp *fillvect_loop
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
	addv v2 v0 v1
	subv v3 v0 v1
	mulv v4 v0 v1
	divv v5 v0 v1
	loadc r0 1
	subs s0 s0 r0
	jmp *loop
.label doscatter
	clrs r0
	clrv v0
	clrv v1
	clrv v2
	loadv v0 *minvect
	idxcomp v1 v0 r0
	scatter v2 v0 v1
	storev v2 *vstore1
	bp
	clrv v0
	clrv v1
	clrv v2
	loadc r1 10
	call *fillvect
	bp
	loadv v1 *vstore1
	addv v3 v1 v0
	bp
.label done
	bp
	halt
.eof
