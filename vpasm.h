typedef struct Inst Inst;
typedef struct Label Label;
typedef struct Instd Instd;
typedef struct Data Data;

struct Inst {
	int len;	// the byte length
	int slen;	// the args array length
	char *raw;	// the raw string pre-tokenize
	char *inst;
	char **args;
	Inst *next;
};

struct Label {
	u32int addr;
	char *name;
	Label *next;
};

struct Data {
	u32int addr;
	u32int dat;
	Data *next;
};

struct Instd {
	u8int otype; // 0 = instruction, 1 = data
	u8int type;
	u8int len;
	u8int inst;
	u8int args[14];
	u8int dat[4];
	Inst *src;
	Data *datsrc;
	Instd *next;
};

extern int maxinst;
extern int debug;
extern int linen;
extern u32int addrcnt;
extern Inst *src;
extern Label *labels;
extern Instd *output;
extern Data *datas;
extern char *panicfn;
extern Instdef insts[];

#define E(a) panicfn=a; if(0){fprint(2, "entering: %s\n", a); }
#define DEBUG(a) if(debug){ fprint(2, "debug: %s\n", a); }

Inst* getinstr(char*);
void normalizeinst(Inst*);
u32int labeltoaddr(char*);
u32int decodeaddr(char*);
void addlabel(char*, u32int);
void handleinst(char*);
void adddata(u32int);
void handlepreproc(char*);
int readsrcline(Biobuf*);
Instdef* getinstdef(char*);
u8int getreg(char*);
void encode32int(u32int, u8int*);
Instd* encodeinst(Inst*);
Instd* encodedata(Data*);
void generateoutput(void);
int dumpoutput(int);
void usage(void);

void dumpsrc(void);
void dumpdata(void);
void dumplabels(void);
