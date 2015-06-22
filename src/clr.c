#include "globals.h"

/*
	thread for handling the cleartext data from the unsecured knx line
		-> read cleartextdata from bus, and hand over to sec lines
		-> get  cleartextdata from sec lines, and write to bus
*/

//extern pthread_mutex_t clr2SecMutexWr[2];
//extern pthread_cond_t  clr2SecCondWr[2];

int clrState = UNINIT;

int checkClrPkg(uint8_t *pkg, uint8_t len)
{
	return 0;
}

void clrWR(void *threadEnv)
{
	while(1)
	{
	//	debug("CLS: blocking(MUTEX)", pthread_self());
		sleep(10);
	}
}

void clrRD(void *threadEnv)
{
	threadEnvClr_t *thisEnv = (threadEnvClr_t *)threadEnv;
	int len = 0;
	knxPacket *packet;
	packet = malloc(sizeof(knxPacket));

	thisEnv->clrFD = EIBSocketURL(thisEnv->socketPath);
	if (!thisEnv->clrFD)
	{
		printf("CLR : opening %s failed\n", thisEnv->socketPath);
		exit(-1);
	}
	#ifdef DEBUG
		printf("CLR : socket opened\n");
	#endif

	while(1)
	{	
		if(EIBOpenVBusmonitor(thisEnv->clrFD) == -1)
		{
			printf("CLR : cannot open KNX Connection\n");
			sleep(5);
		}
		else
		{
		#ifdef DEBUG
			printf("CLR : busmonitor started\n");
		#endif
		break;
		}
	}

	while(1)
	{
		
//		pthread_mutex_lock(&clr2SecMutexWr[0]);
//		pthread_mutex_lock(&clr2SecMutexWr[1]);

		// blocking call to get next package
		len = EIBGetBusmonitorPacket (thisEnv->clrFD, sizeof(*packet), (uint8_t *)packet);
		if (len == -1)
		{
			printf("GET failed\n");
			exit(-1);
		}

		if (len > MAX_CLR_SIZE)
		{
			//debug("discarding oversized clr package",  pthread_self());
		}
		else
		{
			printf("got new knx package\n");
			;;
		}
	}
}

int mainStateMachine(EIBConnection *clrFD)
{
	return 0;
}

int initClr(void *env)
{
	threadEnvClr_t *threadEnvClr = (threadEnvClr_t *) env;
	printf("CLR going home\n");

	return 0;
}
