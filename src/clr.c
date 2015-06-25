#include "globals.h"

/*
	thread for handling the cleartext data from the unsecured knx line
		-> read cleartextdata from bus, and hand over to sec lines
		-> get  cleartextdata from sec lines, and write to bus
*/

//extern pthread_mutex_t clr2SecMutexWr[2];
//extern pthread_cond_t  clr2SecCondWr[2];

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
	uint8_t buffer[BUFSIZE], i=0;

	thisEnv->clrFD = EIBSocketURL(thisEnv->socketPath);

	while(1)
	{	
		if (!thisEnv->clrFD)
		{
			printf("CLR : opening %s failed\n", thisEnv->socketPath);
		}
		else
		{
			#ifdef DEBUG
				printf("CLR : socket opened\n");
			#endif
			if(EIBOpenVBusmonitor(thisEnv->clrFD) == -1)
			{
				printf("CLR : cannot open KNX Connection\n");
			}
			else
			{
				#ifdef DEBUG
					printf("CLR : busmonitor started\n");
				#endif
				break;
			}
		}
		sleep(60);
	}

	while(1)
	{
		
//		pthread_mutex_lock(&clr2SecMutexWr[0]);
//		pthread_mutex_lock(&clr2SecMutexWr[1]);

		/*
 			blocking call to get next package
			leave space for 3 bytes at the begining because of pipe-format(src[0],src[1],secHeader,kxn-frame)
		*/
		
		len = EIBGetBusmonitorPacket (thisEnv->clrFD, BUFSIZE, &buffer[3]);
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
/*
			printf("CLR  : got %d byte: ", len);
			for(i=0;i<len;i++)
			{
				printf("%02X ", buffer[i+3]);
			}
*/
			printf("\n");
			buffer[2] = clrData;
			write(*thisEnv->CLR2Master1PipePtr[WRITEEND], &buffer[0], len+3);
			write(*thisEnv->CLR2Master2PipePtr[WRITEEND], &buffer[0], len+3);
		}
	}
}

void initClr(void *env)
{
	threadEnvClr_t *threadEnvClr = (threadEnvClr_t *) env;
	clrWR(env);
	printf("CLR going home\n");

}
