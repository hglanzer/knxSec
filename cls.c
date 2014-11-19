#include "globals.h"

/*
	thread for handling the cleartext data from the unsecured knx line
		-> read cleartextdata from bus, and hand over to sec lines
		-> get  cleartextdata from sec lines, and write to bus
*/

extern int count;
extern pthread_mutex_t cls2SecMutexWr[2];
extern pthread_cond_t  cls2SecCondWr[2];

extern int newData4Sec;
extern int newData4Cls;

EIBConnection *clsFD;

int clsState = UNINIT;

void sendClsPackage(void)
{
	debug("SEND PACKAGE", pthread_self());
}

int clsSend(void)
{
	while(1)
	{
	//	debug("CLS: blocking(MUTEX)", pthread_self());
		sleep(10);
	}
	return 0;
}

int clsReceive(void)
{
	int len = 0;
	struct packetStruct *packet;
	packet = malloc(sizeof(struct packetStruct));

	while(1)
	{
		// block SEC send call for syncronization
		#ifdef DEBUG
			debug("CLS _____locking", pthread_self());
		#endif
		pthread_mutex_lock(&cls2SecMutexWr[0]);
		pthread_mutex_lock(&cls2SecMutexWr[1]);

		// blocking call to get next package
		len = EIBGetBusmonitorPacket (clsFD, sizeof(*packet), (uint8_t *)packet);
		if (len == -1)
		{
			printf("GET failed\n");
			return -1;
		}
		//printf("%x\n", packet->atNcpiLength & 0x80);
		printf("\tLen = %d / CTRL: %x / %x.%x.%x\n", len, packet->ctrl, \
			AreaAddress(packet->srcAreaLine), LineAddress(packet->srcAreaLine), packet->srcDev);
		newData4Sec = 1;
		#ifdef DEBUG
			debug("CLS _____unlocking", pthread_self());
		#endif
		
		pthread_cond_signal(&cls2SecCondWr[0]);
		pthread_cond_signal(&cls2SecCondWr[1]);

		pthread_mutex_unlock(&cls2SecMutexWr[0]);
		pthread_mutex_unlock(&cls2SecMutexWr[1]);
		count++;
	}

	return 0;
}

int mainStateMachine(EIBConnection *clsFD)
{
	return 0;
}

int initCls(void *env)
{
	struct threadEnvCls_t *arg = (struct threadEnvCls_t *) env;

	int (*clsSendThreadfPtr)(void);
	clsSendThreadfPtr = &clsSend;
	int (*clsRecvThreadfPtr)(void);
	clsRecvThreadfPtr = &clsReceive;

	pthread_t clsRecvThread, clsSendThread;
	
	clsFD = EIBSocketURL(arg->socket);
	#ifdef DEBUG
		debug("cls opening socket", pthread_self());
	#endif
	
	if (clsFD != NULL)
	{
		#ifdef DEBUG
			printf("OK - cls socket opened\n");
		#endif
	}
	else
	{
		printf("NOTOK\n");
		return -1;
	}
	
	if(EIBOpenBusmonitor(clsFD) == 0)
	{
		#ifdef DEBUG
			printf("OK - busmonitor started\n");
		#endif
	}
	else
	{
		printf("cannot open KNX Connection\n");
		return -1;
	}

	if((pthread_create(&clsSendThread, NULL, (void *)clsSendThreadfPtr, NULL)) != 0)
	{
		printf("cls SEND Thread init failed, exit\n");
		return -1;
	}
	if((pthread_create(&clsRecvThread, NULL, (void *)clsRecvThreadfPtr, NULL)) != 0)
	{
		printf("cls RECEIVE Thread init failed, exit\n");
		return -1;
	}

	#ifdef DEBUG
		debug("CLS send/recv threads startet, waiting for kids", pthread_self());
	#endif
	pthread_join(clsRecvThread, NULL);
	pthread_join(clsSendThread, NULL);
	debug("CLS going home", pthread_self());
	return 0;
}
