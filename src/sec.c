#include "globals.h"

extern pthread_mutex_t SecMutexWr[SECLINES];
extern pthread_cond_t  SecCondWr[SECLINES];

uint8_t secBufferWr[SECLINES][BUFSIZE];

uint8_t	secINIT = 0;

//FIXME: this is very insecure.
//	maybe better:
//	read from file to memory, securely delete file, delete buffer after initial phase...?
uint8_t PSK[PSKSIZE] =	"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x30\x31";

/*
	LAYOUT OF FRAMES USED:

	SEC - sync REQUEST:
		-> unauthenticated request to obtain state of globally used seq number
	|--------------------------------------------------------------------------------
	|	 |        |	   	    |		     |	      |	       |	|
	|   CTRL |CTRL-EXT| Source Address  | DestinationAddr| LENGTH |  SECH  |  FC    |  
	|	 | 	  | 	   	    |		     |	      |	       |	|
	|--------------------------------------------------------------------------------

	SEC - sync RESPONSE:
		-> authenticated/encrypted(PSK) response to syncronize requester with global counter + global key
	|--------------------------------------------------------------------------------------------------------------------------------------------------------|
	|	 |        |	   	    |		     |	      |	       |			|			|			|	 |
	|   CTRL |CTRL-EXT| Source Address  | DestinationAddr| LENGTH |  SECH  | Global Sequence Counter|	GLOBAL KEY	|      MAC (k_PSK)	|   FC   |  
	|	 | 	  | 	   	    |		     |	      |	       |	4-6 Byte	|	32 BYTE		|	4-6 Byte	|	 |
	|--------------------------------------------------------------------------------------------------------------------------------------------------------|

----------------------------------------------------------------------------------------------------------------------------------------------------
	
----------------------------------------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------------------
*/

EIBConnection *secFD[SECLINES];

/*
	harald glanzer
	secure line thread
*/

void printKey(uint8_t *key, uint8_t keysize)
{
	int i = 0;
	
	printf("0x");
	for(i = 0; i < keysize; i++)
	{
		printf("%X", key[i]);
	}
	printf("\n");
}

/*
	gets pointer to dataframe
	returns type or INVALID if pkg is unknown / corrupted
*/
int checkPkg(uint8_t *pkg)
{
	uint8_t type = INVALID;

	

	return type;
}

/*
	sec SEND thread
*/
int secSend(void *env)
{
	struct threadEnvSec_t *thisEnv = (struct threadEnvSec_t *)env;
	printf("SEC%d: send ready", thisEnv->id);

	while(1)
	{
		// wait for new data to send over secure line
		pthread_mutex_lock(&SecMutexWr[thisEnv->id]);
		pthread_cond_wait(&SecCondWr[thisEnv->id], &SecMutexWr[thisEnv->id]);

		#ifdef DEBUG
			printf("SEC%d: sending payload = 0x%X\n", thisEnv->id, secBufferWr[thisEnv->id][0]);
		#endif

		pthread_mutex_unlock(&SecMutexWr[thisEnv->id]);
	}
	return 0;
}

/*
	sec RECEIVE thread
							|------> secSend			(msg messages)
							|
	datapath:	secReceive -> secMaster --------|
							|
							|------> checkDup ----> clrSend		(knx traffic)
*/
int secReceive(void *env)
{
	int rc = 0, i = 0;
	struct threadEnvSec_t *thisEnv = (struct threadEnvSec_t *)env;
	while(1)
	{
		rc = EIBGetBusmonitorPacket(thisEnv->secFD, sizeof(thisEnv->localRDBuf), thisEnv->localRDBuf);
		if(rc == -1)
		{
			debug("secReceive(): EIBGetBusMonitorPacket() FAILED", pthread_self());
		}
		else
		{
			for(i=0; i<rc;i++)
				printf("%X ", thisEnv->localRDBuf);

			decodeFrame(thisEnv->localRDBuf);
		}
		
	}
	return 0;
}

/*
	main statemachine for SEC master thread
		- negotiates key
		- renewes key
*/
void keyInit(void *env)
{
	int selectRC = 0;
	struct timeval syncTimeout;

	fd_set set;
	struct threadEnvSec_t *thisEnv = (struct threadEnvSec_t *)env;
	uint8_t buffer[BUFSIZE]; 

	thisEnv->state = INIT;

	/*	
		1a) send sync request		(cleartext)
		1b) wait for sync response	-> obtain global counter(auth)
	*/

	while(1)
	{
		switch(thisEnv->state)
		{
			case INIT:
				#ifdef DEBUG
					printf("SEC%d INIT\n", thisEnv->id);
				#endif

				thisEnv->retryCount = 0;
				FD_ZERO(&set);
				FD_SET(thisEnv->Read2MasterPipe[READEND], &set);
				
				thisEnv->state = SYNC;
			break;
			case SYNC:
				#ifdef DEBUG
					printf("SEC%d: sending %d. sync req\n", thisEnv->id, thisEnv->retryCount);
				#endif

				pthread_mutex_lock(&SecMutexWr[thisEnv->id]);

				secBufferWr[thisEnv->id][0] = 0xfe;

				pthread_cond_signal(&SecCondWr[thisEnv->id]);
				pthread_mutex_unlock(&SecMutexWr[thisEnv->id]);

				syncTimeout.tv_sec = SYNCTIMEOUT_SEC;
				syncTimeout.tv_usec = 0;	

				selectRC = select(FD_SETSIZE, &set, NULL, NULL, &syncTimeout);

				// timeout
				if(selectRC == 0)
				{
					thisEnv->retryCount++;
		
					/*
						seems like there is no other device reachable / online(on this SECline)
							* reset globalCounter
							* choose new global key
							* generate DH parameters	
					*/
					if(thisEnv->retryCount == SYNC_RETRIES)
					{
						thisEnv->state = CHOOSE_KEY;
						#ifdef DEBUG
							printf("SEC%d: key master sync give up\n", thisEnv->id);
						#endif
					}
					else
					{
						#ifdef DEBUG
							printf("SEC%d: key master sync timeout\n", thisEnv->id);
						#endif
					}
					
				}
				// error occured
				else if(selectRC < 0)
				{
					// error
					printf("%s - ", strerror(errno));
					#ifdef DEBUG
						printf("SEC%d: select() error\n", thisEnv->id);
					#endif
				}
				// data received
				else
				{
					// fd ready
					#ifdef DEBUG
						printf("SEC%d: sync response\n", thisEnv->id);
					#endif
					read(thisEnv->Read2MasterPipe[READEND], &buffer[0], sizeof(buffer));

					if(1)	// check package if valid response
					{

					}
					else	// no valid SYNC RESPONSE - retry
					{

					}
				}
			break;

			// looks like this node is alone - reset global counter
			// DISABLED: choose global key randomly from key space
			case CHOOSE_KEY:
				thisEnv->globalCount = 0;
				thisEnv->state = READY;
				/*
				if(RAND_bytes(thisEnv->globalKey, GKSIZE) != 1)
				{
					#ifdef DEBUG
						printf("SEC%d: key select failed\n", thisEnv->id);
					#endif
				}
				#ifdef DEBUG
					printf("SEC%d: resetting global counter\n", thisEnv->id);
					printKey(thisEnv->globalKey, GKSIZE);
				#endif
				*/
			break;

			// node is ready to process datagrams
			case READY:
				#ifdef DEBUG
					printf("SEC%d: key master ready\n", thisEnv->id);
				#endif
				sleep(10);
			break;

			default:
			assert(0);
		}
	};
				
	debug("key Master EXIT", pthread_self());
}

/*
	entry point function from main thread - called 2 times
	generates 1 RECEIVE and 1 SEND thread
*/
int initSec(void *threadEnv)
{
	struct threadEnvSec_t *thisEnv = (struct threadEnvSec_t *)threadEnv;

	int (*secRecvThreadfPtr)(void *);
	secRecvThreadfPtr = &secReceive;

	int (*secSendThreadfPtr)(void *);
	secSendThreadfPtr = &secSend;

	struct threadEnvSec_t *threadEnvSec = (struct threadEnvSec_t *)threadEnv;

	pthread_t secRecvThread, secSendThread;
	printf("%u sock = %s id = %d\n", (unsigned)pthread_self(),threadEnvSec->socketPath, threadEnvSec->id);

	thisEnv->secFD = EIBSocketURL(thisEnv->socketPath);
	if(EIBOpenBusmonitor(thisEnv->secFD) == -1)
	{
		printf("SEC%d: cannot open KNX Connection\n", thisEnv->id);
		return -1;
	}
	else
	{
		printf("SEC%d: KNX Connection opened\n", thisEnv->id);
	}

	// create READ thread
	if((pthread_create(&secRecvThread, NULL, (void *)secRecvThreadfPtr, (void *)threadEnvSec)) != 0)
	{
		printf("sec RECEIVE Thread init failed, exit\n");
		return -1;
	}

	// create WRITE thread
	if((pthread_create(&secSendThread, NULL, (void *)secSendThreadfPtr, (void *)threadEnvSec)) != 0)
	{
		printf("sec SEND Thread init failed, exit\n");
		return -1;
	}

	/*
		pipe is used for communication: secRead -> secKeymaster
		with pipes select() with timeouts can be used!
	*/

	if(pipe(threadEnvSec->Read2MasterPipe) == -1)
	{
		debug("pipe() failed, exit", pthread_self());
		return 0;
	}

	#ifdef DEBUG
		printf("SEC%d / pipFD: %d <- %d\n", threadEnvSec->id, threadEnvSec->Read2MasterPipe[READEND], threadEnvSec->Read2MasterPipe[WRITEEND]);
	#endif

	keyInit(threadEnvSec);
	
	pthread_join(secRecvThread, NULL);
	pthread_join(secRecvThread, NULL);
	printf("goiing home\n");
	return 0;
}
