typedef struct Instdef Instdef;

struct Instdef {
	char *name;
	u8int type;
	u8int op;
	u8int len;
};

enum {
	CATIMPL		= 0xf0,
	CATSMEM		= 0x00,
	CATVMEM		= 0x10,
	CATCTL		= 0x20,
	CATSMATH	= 0x30,
	CATVMATH	= 0x40,
};

enum {
	ARGCONT,	// constant
	ARGREG,		// register
	ARGADDR,	// address
};

enum {
	// scalar mem
	LOADS	= 0x10,
	STORES	= 0x11,
	MOVES	= 0x12,
	RLOADS	= 0x13,
	RSTORES	= 0x14,
	PUSH	= 0x15,
	POP		= 0x16,
	LOADC	= 0x17,
	CLRS	= 0x18,
	LOADB	= 0x19,
	STOREB	= 0x1a,
	RLOADB	= 0x1b,
	RSTOREB	= 0x1c,
	// vector mem
	LOADV	= 0x10,
	STOREV	= 0x11,
	MOVEV	= 0x12,
	RLOADV	= 0x13,
	RSTOREV	= 0x14,
	RLOADSV	= 0x15,
	RSTORESV= 0x16,
	CLRV	= 0x17,
	GATHER	= 0x18,
	SCATTER = 0x19,
	IDXCOMP = 0x20,
	// control flow
	NOP		= 0xff,
	JMP		= 0x10,
	RJMP	= 0x11,
	JMPR	= 0x12,
	RJMPR	= 0x13,
	CLRF	= 0x14,
	CJMP	= 0x15,
	CRJMP	= 0x16,
	CJMPR	= 0x17,
	CRJMPR	= 0x18,
	SETSYS	= 0x19,
	SYSCALL	= 0x1a,
	CALL	= 0x1b,
	RCALL	= 0x1c,
	RETURN	= 0x1d,
	RCALLR	= 0x1e,
	HALT	= 0x1f,
	TSTART	= 0x20,
	TSTOP	= 0x21,
	// scalar math
	EQS		= 0x10,
	GTS		= 0x11,
	LTS		= 0x12,
	ADDS	= 0x13,
	SUBS	= 0x14,
	MULS	= 0x15,
	DIVS	= 0x16,
	MODS	= 0x17,
	NANDS	= 0x18,
	// vector math
	EQV		= 0x10,
	GTV		= 0x11,
	LTV		= 0x12,
	ADDV	= 0x13,
	SUBV	= 0x14,
	MULV	= 0x15,
	DIVV	= 0x16,
	MODV	= 0x17,
	// implementation specific
	BP		= 0x10,
	PRINT	= 0x11,
	RFMT	= 0x12,
};
