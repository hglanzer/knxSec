/*
	harald glanzer
	global definitions / includes

*/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<eibclient.h>
#include<assert.h>
#include<getopt.h>
#include<time.h>
#include<signal.h>
#include<pthread.h>
#include<sys/select.h>
#include<string.h>
#include<errno.h>
#include <openssl/rand.h>

#include "master.h"
#include "cls.h"
#include "sec.h"
#include "thread.h"
#include "knx.h"

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
