#include "globals.h"

/*
	thread for handling the cleartext data from the unsecured knx line
		-> read cleartextdata from bus, and hand over to sec lines
		-> get  cleartextdata from sec lines, and write to bus
*/

//extern pthread_mutex_t clr2SecMutexWr[2];
//extern pthread_cond_t  clr2SecCondWr[2];

extern pthread_mutex_t globalMutex;

void ctrInt2Str(uint32_t ctrInt, uint8_t *ctr)
{
	uint8_t i=0;
	for(i=INDCOUNTSIZE;i>0;i=i-1)
	{
		ctr[i-1] = ctrInt % 256;
		ctrInt = ctrInt / 256;
	}
}

int str2CtrInt(uint8_t *ctr)
{
	uint8_t i = 0;
	uint32_t tmp = 0, exp=1;

	for(i=INDCOUNTSIZE; i>0;i=i-1)
	{
		tmp = tmp + ctr[i-1] * exp;
		exp = exp*256;
	}
	return tmp;
}

int searchSRC(void *env, uint8_t *src, uint32_t updateVal)
{
	threadEnvClr_t *thisEnv = (threadEnvClr_t *)env;
	uint8_t i=0;
	eibaddr_t srcEIBtmp;

	srcEIBtmp = ((src[0]<<8) | (src[1]));

	printf("CLR: searching counter for srcEIB %d / ", srcEIBtmp);
	for(i=0;i<10;i++)
	{
		if(thisEnv->indCtr[i].srcEIB == srcEIBtmp)
		{
			printf("found, CTR = %d", thisEnv->indCtr[i].indCount);
			if(updateVal == 0)
				thisEnv->indCtr[i].indCount++;
			else
			{
				if(updateVal > thisEnv->indCtr[i].indCount)
				{
					thisEnv->indCtr[i].indCount = updateVal;
					return TRUE;
				}
				else
				{
					return FALSE;
				}

			}
			return thisEnv->indCtr[i].indCount;
		}
		if(thisEnv->indCtr[i].srcEIB == 0x00)
		{
			printf("not found, set CTR = 1\n");
			thisEnv->indCtr[i].indCount = 0x01;
			thisEnv->indCtr[i].srcEIB = srcEIBtmp;
			return 0x01;
		}
	}

	// the loop finished without finding an existing entry for srcEIB and could not find an unused entry too -> array too smalL
	// FIXME: dynamic memory handling, linked list...?
	printf("CLR: add ind counter out of memory, fixme\n\n");
	exit(-1);
}
void clrWR(void *threadEnv)
{
	threadEnvClr_t *thisEnv = (threadEnvClr_t *)threadEnv;
	uint8_t rc = 0, i=0;
	uint8_t buffer[BUFSIZE];
	int indCntTmp;
	eibaddr_t srcEIBtmp;
	while(1)
	{
		rc = read(thisEnv->SECs2ClrPipe[READEND], &buffer[0], BUFSIZE);	// FIXME - non-blocking

		pthread_mutex_lock(&globalMutex);
		printf("CLR-WR: got input ");
		
		indCntTmp = str2CtrInt(&buffer[0]);

		for(i=0;i<rc;i++)
			printf("%02X ", buffer[i]);

		if((buffer[4] >> 8) && 0x01)
		{
			printf("STD\n");
			rc = searchSRC(threadEnv, &buffer[5], indCntTmp);
		}
		else
		{
			printf("EXT\n");
			rc = searchSRC(threadEnv, &buffer[6], indCntTmp);
		}

		printf(", indCtr = %04d: ", indCntTmp);
				
		if(rc)
		{
			printf("FORWARDING TO CLR\n\n");
		}					
		else
		{
			printf("DISCARDING DUPLICATE\n\n");
		}
		

		pthread_mutex_unlock(&globalMutex);
	}
}

void clrRD(void *threadEnv)
{
	threadEnvClr_t *thisEnv = (threadEnvClr_t *)threadEnv;
	int len = 0;
	uint8_t buffer[BUFSIZE], i=0;
	uint8_t ctrBuf[INDCOUNTSIZE];
	knxPacket tmpPkt;
	uint8_t ctrInt = 0;

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
		
		/*
 			blocking call to get next package
			leave space for 3 bytes at the begining because of pipe-format(src[0],src[1],secHeader,kxn-frame)
			append the actual ind. counter for this SRC after the packet
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
			printf("CLR  : got %d byte: ", len);
			for(i=0;i<len;i++)
			{
				printf("%02X ", buffer[i+3]);
			}
				printf("\n");
			decodeFrame(&buffer[3], &tmpPkt);

			if(tmpPkt.type == stdFrame)
			{
				// copy the DEST bytes + type to beginning of pipe-buf
				buffer[0] = buffer[6];
				buffer[1] = buffer[7];
				buffer[2] = clrDataSTD;

				ctrInt = searchSRC(threadEnv, &buffer[4], 0);
				ctrInt2Str(ctrInt, &ctrBuf[0]);

				// append the ind counter 
				buffer[len+3+0] = ctrBuf[0];
				buffer[len+3+1] = ctrBuf[1];
				buffer[len+3+2] = ctrBuf[2];
				buffer[len+3+3] = ctrBuf[3];

				// FIXME: mutex...?
				write(*thisEnv->CLR2Master1PipePtr[WRITEEND], &buffer[0], len+3+4);
				write(*thisEnv->CLR2Master2PipePtr[WRITEEND], &buffer[0], len+3+4);
			}
			else
			{
				printf("TODO: handle CRL extended frames\n");
			}
		}
	}
}

void initClr(void *env)
{
	threadEnvClr_t *threadEnvClr = (threadEnvClr_t *) env;
	clrWR(env);
	printf("CLR going home\n");

}
