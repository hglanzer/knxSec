#include "globals.h"

/*
	thread for handling the cleartext data from the unsecured knx line
		-> read cleartextdata from bus, and hand over to sec lines
		-> get  cleartextdata from sec lines, and write to bus
*/

extern int count;
extern pthread_mutex_t sec2ClsMutexWr;
extern pthread_mutex_t sec2ClsMutexRd;
extern pthread_mutex_t cls2SecMutexWr;
extern pthread_mutex_t cls2SecMutexRd;

extern int newData4Sec;
extern int newData4Cls;

EIBConnection *master;

int clsState = UNINIT;

void sendClsPackage(void)
{
	debug("SEND PACKAGE");
}

int clsSend(void)
{
	while(1)
	{
		printf("CLS: blocking(MUTEX)\n");
		sleep(1);
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
		printf("cls: locking\n");
		pthread_mutex_lock(&cls2SecMutexRd);
		// blocking call to get next package
		len = EIBGetBusmonitorPacket (master, sizeof(*packet), (uint8_t *)packet);
		if (len == -1)
		{
			printf("GET failed\n");
			return -1;
		}
		printf("%x\n", packet->atNcpiLength & 0x80);
		printf("Len = %d / CTRL: %x / %x.%x.%x", len, packet->ctrl, \
			AreaAddress(packet->srcAreaLine), LineAddress(packet->srcAreaLine), packet->srcDev);
		newData4Sec = 1;
		printf("cls: unlocking\n");
		pthread_mutex_unlock(&cls2SecMutexRd);
		pthread_mutex_lock(&cls2SecMutexWr);
		count++;
	}

	return 0;
}

void debug(char *str)
{
	printf("CLS %u: %s\n", (unsigned)pthread_self(), str);
}

int mainStateMachine(EIBConnection *master)
{
	return 0;
}

int initCls(void *env)
{
	int rc = 0;
	
	struct threadEnvCls_t *arg = (struct threadEnvCls_t *) env;

	int (*clsSendThreadfPtr)(void);
	clsSendThreadfPtr = &clsSend;
	int (*clsRecvThreadfPtr)(void);
	clsRecvThreadfPtr = &clsReceive;

	pthread_t clsRecvThread, clsSendThread;

	master = EIBSocketURL(arg->socket);
	debug("opening socket");
	debug(arg->socket);


	if (master != NULL)
		printf("OK - socket opened\n");
	else
	{
		printf("NOTOK\n");
		return -1;
	}
	
	if(EIBOpenBusmonitor(master) == 0)
	{
		printf("OK - busmonitor started\n");
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

	pthread_join(clsRecvThread, NULL);
	pthread_join(clsSendThread, NULL);
	debug("going home");
	return 0;
}
