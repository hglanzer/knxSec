/*
	harald glanzer
	global definitions / includes

*/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<math.h>
#include<time.h>
#include<assert.h>
#include<getopt.h>
#include<time.h>
#include<signal.h>
#include<pthread.h>
#include<sys/select.h>
#include<string.h>
#include<errno.h>
#include<openssl/rand.h>
#include<openssl/ssl.h>
#include<openssl/err.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>

#include"eibclient.h"
#include"master.h"
#include"clr.h"
#include"sec.h"
#include"thread.h"
#include"knx.h"
#include"cryptoHelper.h"

// 	MAKROS
#define AreaAddress(adr) ((uint8_t)(adr >> 4 ))
#define LineAddress(adr) ((uint8_t)(adr & 0x0F))

#define isGroupaddress(adr) ((uint8_t)(adr & (1<<8)))

//	STATES
#define UNINIT		0
#define INITIALIZE	1
#define INIT_DONE	2

#define	CLSBUFSIZE	256
#define	SECBUFSIZE	256

#define SEC1		0
#define SEC2		1

#define SYNCRETRY	3

// FIXME	
//const char hn[] = "SHA256";
	
//	message queue stuff
#define	MSGKEY_SEC2WR	123
#define MSG_PERM	0666
#define MSG_TYPE	1

struct msgbuf_t {
	long mtype;       /* message type, must be > 0 */
	char buf[256];    /* message data */
};
