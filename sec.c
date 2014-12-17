#include "globals.h"

extern pthread_mutex_t clr2SecMutexWr[SECLINES];
extern pthread_cond_t  clr2SecCondWr[SECLINES];

extern int newData4Sec;
extern int newData4Cls;

extern int count;

uint8_t state = BOOTED;
uint8_t	secINIT = 0;

EIBConnection *secFD[SECLINES];

/*
	harald glanzer
	secure line thread
*/

/*
	sec receive thread
*/
int secSend(void *myID)
{
	uint8_t id = *(uint8_t *)myID;

	while(1)
	{
		// wait for new data to send over secure line
		pthread_mutex_lock(&clr2SecMutexWr[id]);
		pthread_cond_wait(&clr2SecCondWr[id], &clr2SecMutexWr[id]);

		if(secINIT)
		{
			#ifdef DEBUG
				if(id == 0)
					debug("SEC: blocking(MUTEX 0)", pthread_self());
				else
					debug("SEC: blocking(MUTEX 1)", pthread_self());
			#endif
	
			debug("SEC: sending data", pthread_self());
	
		}
		else
		{
			debug("no key established", pthread_self());
		}
		pthread_mutex_unlock(&clr2SecMutexWr[id]);
	}
	return 0;
}

/*
	sec receive thread
*/
int secReceive(void *myID)
{
	uint8_t id = *(uint8_t *)myID;

	printf("ID %d , waiting for data from EIBD\n", id);
	while(1)
	{
		//debug("SEC: blocking(EIBD)", pthread_self());
		sleep(10);
	}
	return 0;
}

/*
	main statemachine for SEC master thread
		- negotiates key
		- renewes key
*/
void keyInit(void)
{
	int i = 0;

	
	// stage 1
		// send HELLO message to SEC1 + SEC2
		// wait for ANSWER on SEC1 + SEC2 (set timeout / alarm)

	// stage 2
		// SEND CHALLENGE
		// wait for challenge RESPONSE(set timeout / alarm)

	// retry
	for(i = 0; i <= SYNCRETRY; i++)
	{
		switch(state)
		{
			case BOOTED:
			break;
	
			case HELO:
			break;
		
			case CHALLENGE:
			break;
	
			default:
			assert(0);
		}
	}
}

/*
	entry point function from main thread - called 2 times
	generates 1 RECEIVE and 1 SEND thread
*/
int initSec(void *str)
{
	int (*secRecvThreadfPtr)(void *);
	secRecvThreadfPtr = &secReceive;

	int (*secSendThreadfPtr)(void *);
	secSendThreadfPtr = &secSend;

	struct threadEnvSec_t *threadEnvSec = (struct threadEnvSec_t *)str;

	pthread_t secRecvThread, secSendThread;
	printf("this is sec %u, with args sock = %s id = %d\n", (unsigned)pthread_self(),threadEnvSec->socket, threadEnvSec->id);

	if((pthread_create(&secRecvThread, NULL, (void *)secRecvThreadfPtr, &threadEnvSec->id)) != 0)
	{
		printf("sec RECEIVE Thread init failed, exit\n");
		return -1;
	}
	if((pthread_create(&secSendThread, NULL, (void *)secSendThreadfPtr, &threadEnvSec->id)) != 0)
	//if((pthread_create(&secSendThread, NULL, (void *)secSendThreadfPtr, &threadEnvSec->id)) != 0)
	{
		printf("sec SEND Thread init failed, exit\n");
		return -1;
	}
	keyInit();
	
	pthread_join(secRecvThread, NULL);
	pthread_join(secRecvThread, NULL);
	printf("goiing home\n");
	return 0;
}
