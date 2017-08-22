#include "regs.h"	// bad practice

enum {
	VECEQ,
	VECGT,
	VECLT,
	VECADD,
	VECSUB,
	VECMUL,
	VECDIV,
	VECMOD,
	VECLOAD,
	VECSTORE,
	VECCLR,
};

typedef struct Cpu Cpu;
typedef struct Vect Vect;
typedef struct Vinst Vinst;
typedef struct Inst Inst;
typedef struct Regr Regr;

struct Vect {
	u32int dat[VECLEN];
};

struct Cpu {
	QLock;
	// the registers
	u32int g_regs[16];
	Vect v_regs[8];
	u32int c_regs[5];
	// u32int op, pc, sp, sy, fl;
	Channel *vunit;
	int pause;
	int cont;
	// memory i/o
	u8int (*memread)(u32int);
	void (*memwrite)(u8int*, u32int);
};

struct Vinst {
	Cpu *c;
	int inst;
	u32int *dest, *src1, *src2;
	Vect *vsrc1, *vsrc2, *vdest;
	Channel *reply;
};

struct Inst {
	u8int type;
	u8int op;
	u32int args[3];
};

struct Regr {
	u8int type; // scalar or vector
	Vect *v;
	u32int *s;
};

// mem.c
extern u8int *memory;
extern u32int memlen;
void meminit(u32int);
u8int memread(u32int);
void memwrite(u8int*, u32int);

// main.c
extern Cpu *cpu0;
void doimplop(Cpu*, Inst*);

// cpu.c
Regr* _getregister(Cpu*, u32int);
u32int* getregister(Cpu*, u32int);
void fetchdecode(Cpu*, Inst*);
void execute(Cpu*, Inst*);
void readreg(Cpu*, u32int*, u32int);
void writereg(Cpu*, u32int*, u32int);
void doscalarmemop(Cpu*, Inst*);
void docontrolop(Cpu*, Inst*);
void doscalarmathop(Cpu*, Inst*);
void startexec(Cpu*, u32int);
void cpuhalt(void);
void startexec(Cpu*, u32int);
Cpu* makecpu(void);

// vcpu.c
void readvect(Cpu*, Vect*, u32int);
void writevect(Cpu*, Vect*, u32int);
void startvectworker(Cpu*);
void vectworker(void*);
Vect* getvreg(Cpu*, u32int);
void sendvectop(Cpu*, Inst*);
void sendvectmemop(Cpu*, Inst*);
void sendvectmathop(Cpu*, Inst*);

// debug.c
extern int debug;
void dprint(char*);
void panic(char*);
