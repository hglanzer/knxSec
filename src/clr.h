int initClr(void *);
void clrRD(void *);
void clrWR(void *);
int mainStateMachine(EIBConnection *master);

#define MAX_CLR_SIZE	64

#define	DATA_STD	0x00
#define DATA_EXT	0x01
#define DATA_POLL	0x02
#define DATA_ACK	0x03
#define DATA_INVALID	0x04
