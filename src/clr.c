#include "globals.h"

/*
	thread for handling the cleartext data from the unsecured knx line
		-> read cleartextdata from bus, and hand over to sec lines
		-> get  cleartextdata from sec lines, and write to bus
*/

//extern pthread_mutex_t clr2SecMutexWr[2];
//extern pthread_cond_t  clr2SecCondWr[2];

EIBConnection *clrFD;

int clrState = UNINIT;

void sendClrPackage(void)
{
	debug("SEND PACKAGE", pthread_self());
}

int clrSend(void)
{
	while(1)
	{
	//	debug("CLS: blocking(MUTEX)", pthread_self());
		sleep(10);
	}
	return 0;
}

int checkClrPkg(uint8_t *pkg, uint8_t len)
{
	uint8_t type = INVALID;

	return type;
}

int clrReceive(void)
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
//		pthread_mutex_lock(&clr2SecMutexWr[0]);
//		pthread_mutex_lock(&clr2SecMutexWr[1]);

		// blocking call to get next package
		len = EIBGetBusmonitorPacket (clrFD, sizeof(*packet), (uint8_t *)packet);
		if (len == -1)
		{
			printf("GET failed\n");
			return -1;
		}

		if (len > MAX_CLR_SIZE)
		{
			debug("discarding oversized clr package",  pthread_self());
		}
		else
		{
			//printf("%x\n", packet->atNcpiLength & 0x80);
			printf("\tLen = %d / CTRL: %x / %x.%x.%x\n", len, packet->ctrl, \
				AreaAddress(packet->srcAreaLine), LineAddress(packet->srcAreaLine), packet->srcDev);
			#ifdef DEBUG
				debug("CLS _____unlocking", pthread_self());
			#endif
			
//			pthread_cond_signal(&clr2SecCondWr[0]);
//			pthread_cond_signal(&clr2SecCondWr[1]);
	
//			pthread_mutex_unlock(&clr2SecMutexWr[0]);
//			pthread_mutex_unlock(&clr2SecMutexWr[1]);
		}
	}

	return 0;
}

int mainStateMachine(EIBConnection *clrFD)
{
	return 0;
}

int initClr(void *env)
{
	struct threadEnvClr_t *arg = (struct threadEnvClr_t *) env;

	int (*clrSendThreadfPtr)(void);
	clrSendThreadfPtr = &clrSend;
	int (*clrRecvThreadfPtr)(void);
	clrRecvThreadfPtr = &clrReceive;

	pthread_t clrRecvThread, clrSendThread;

	debug("THIS IS **CLR**", pthread_self());
	clrFD = EIBSocketURL(arg->socket);
	
	if (!clrFD)
	{
		printf("%s", arg->socket);
		debug("CLR: EIBSocketURL() failed", pthread_self());
		return -1;
	}
	#ifdef DEBUG
		printf("OK - clr socket opened\n");
	#endif
	
	if(EIBOpenBusmonitor(clrFD) == -1)
	{
		printf("cannot open KNX Connection\n");
		return -1;
	}

	#ifdef DEBUG
		printf("OK - busmonitor started\n");
	#endif

	if((pthread_create(&clrSendThread, NULL, (void *)clrSendThreadfPtr, NULL)) != 0)
	{
		printf("clr SEND Thread init failed, exit\n");
		return -1;
	}
	if((pthread_create(&clrRecvThread, NULL, (void *)clrRecvThreadfPtr, NULL)) != 0)
	{
		printf("clr RECEIVE Thread init failed, exit\n");
		return -1;
	}

	#ifdef DEBUG
		debug("CLS send/recv threads startet, waiting for kids", pthread_self());
	#endif
	pthread_join(clrRecvThread, NULL);
	pthread_join(clrSendThread, NULL);
	debug("CLS going home", pthread_self());
	return 0;
}
