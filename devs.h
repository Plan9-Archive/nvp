typedef struct Device Device;

struct Device {
	QLock;
	int active;
	void (*init)(void);
	u8int (*read)(void);	// byte 0 read
	void (*write)(u8int);	// byte 0 write
	u8int (*status)(void);	// byte 1 read
	void (*cmd)(u8int);	// byte 1 write
	u8int dat[14];
};

extern Device *devs[];
extern Device console;
extern Device disk;

u8int devread(Device*, u8int);
void devwrite(Device*, u8int, u8int);

// console.c
void consinit(void);

// disk.c
void initdisk(int, u32int);
