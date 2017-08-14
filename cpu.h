enum {
	VECLEN = 8,		// vector length (in scalar registers)
};

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

enum {
	// gp registers
	R0 = 0x10, R1 = 0x11, R2 = 0x12, R3 = 0x13, R4 = 0x14,
	R5 = 0x15, R6 = 0x16, R7 = 0x17,
	S0 = 0x18, S1 = 0x19, S2 = 0x1a, S3 = 0x1b, S4 = 0x1c,
	S5 = 0x1d, S6 = 0x1e, S7 = 0x1f,
	// vector registers
	V0 = 0x20, V1 = 0x21, V2 = 0x22, V3 = 0x23, V4 = 0x24,
	V5 = 0x25, V6 = 0x26, V7 = 0x27,
	// control registers
	OP = 0x30,
	PC = 0x31,
	SP = 0x32,
	FL = 0x33,
	SY = 0x34,
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
