enum {
	VECLEN = 8,		// vector length (in scalar registers)
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
	TI = 0x35,
	TR = 0x36,
	TL = 0x37,
	TC = 0x38,
};
