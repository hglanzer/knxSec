#include "globals.h"

extern pthread_mutex_t cls2SecMutexRd;
extern pthread_mutex_t cls2SecMutexWr;
extern pthread_mutex_t sec2ClsMutexRd;
extern pthread_mutex_t sec2ClsMutexWr;

extern int newData4Sec;
extern int newData4Cls;

extern int count;

/*
	harald glanzer
	secure line thread
*/

/*
	sec receive thread
*/
int secSend()
{
	while(1)
	{
		printf("SEC: blocking(MUTEX)\n");

		pthread_mutex_lock(&cls2SecMutexRd);
		pthread_mutex_lock(&cls2SecMutexWr);

		if(newData4Sec)
			printf("___________SEC: sending data\n");
		else
			printf("SEC: nothing to send\n");

		newData4Sec = 0;
		pthread_mutex_unlock(&cls2SecMutexRd);
		pthread_mutex_unlock(&cls2SecMutexWr);
	}
	return 0;
}

/*
	sec receive thread
*/
int secReceive(void)
{
	while(1)
	{
		printf("SEC: blocking(EIBD)\n");
		sleep(1);
	}
	return 0;
}

/*
	callable from main thread - 
	main thread writes data which must be sent through secline
*/
void test(char *str)
{
	printf("this is secThread %u, got arg %s\n", (unsigned)pthread_self, str);
}

/*
	entry point function from main thread
*/
int initSec(void *arg)
{
	int (*secRecvThreadfPtr)(void);
	secRecvThreadfPtr = &secReceive;

	int (*secSendThreadfPtr)(void);
	secSendThreadfPtr = &secSend;

	pthread_t secRecvThread, secSendThread;
	printf("this is sec %u\n", (unsigned)pthread_self());
	
	if((pthread_create(&secRecvThread, NULL, (void *)secRecvThreadfPtr, NULL)) != 0)
	{
		printf("sec RECEIVE Thread init failed, exit\n");
		return -1;
	}
	if((pthread_create(&secSendThread, NULL, (void *)secSendThreadfPtr, NULL)) != 0)
	{
		printf("sec SEND Thread init failed, exit\n");
		return -1;
	}
	
	pthread_join(secRecvThread, NULL);
	pthread_join(secRecvThread, NULL);

	printf("goiing home\n");
	return 0;
}
