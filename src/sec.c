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
	sec receive thread
*/
int secSend(void *env)
{
	struct threadEnvSec_t *thisEnv = (struct threadEnvSec_t *)env;
	printf("ID = %d ", thisEnv->id);
	debug("SEC SEND ready", pthread_self());

	while(1)
	{
		// wait for new data to send over secure line
		pthread_mutex_lock(&SecMutexWr[thisEnv->id]);
		pthread_cond_wait(&SecCondWr[thisEnv->id], &SecMutexWr[thisEnv->id]);

		printf("ID = %d, 0x%X", thisEnv->id, secBufferWr[thisEnv->id][0]);
		debug("sending payload", pthread_self());

		pthread_mutex_unlock(&SecMutexWr[thisEnv->id]);
	}
	return 0;
}

/*
	sec receive thread
*/
int secReceive(void *env)
{
	struct threadEnvSec_t *thisEnv = (struct threadEnvSec_t *)env;
	#ifdef DEBUG
		printf("SEC%d , waiting for data from EIBD\n", thisEnv->id);
	#endif
	while(1)
	{
//		debug("SEC: blocking(EIBD)", pthread_self());
		sleep(10);
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
	struct timeval joinTimeout;

	fd_set set;
	struct threadEnvSec_t *thisEnv = (struct threadEnvSec_t *)env;
	uint8_t buffer[BUFSIZE]; 

	thisEnv->state = INIT;

	/*	
		1a) send sync request		(cleartext)
		1b) wait for sync response	-> obtain global counter(auth)

		2a) send join request		(auth)
		2b) wait for join response	-> obtain global key(auth+enc)

	*/

	while(1)
	{
		switch(thisEnv->state)
		{
			case INIT:

				debug("key Master: INIT", pthread_self());

				thisEnv->retryCount = 0;
				FD_ZERO(&set);
				FD_SET(thisEnv->Read2MasterPipe[READEND], &set);
				
				thisEnv->state = SYNC;
			break;
			case SYNC:
				debug("key Master: SYNC / select() / sending sync req", pthread_self());

				pthread_mutex_lock(&SecMutexWr[thisEnv->id]);

				secBufferWr[thisEnv->id][0] = 0xfe;

				pthread_cond_signal(&SecCondWr[thisEnv->id]);
				pthread_mutex_unlock(&SecMutexWr[thisEnv->id]);

				syncTimeout.tv_sec = SYNCTIMEOUT_SEC;
				syncTimeout.tv_usec = 0;	

				selectRC = select(FD_SETSIZE, &set, NULL, NULL, &syncTimeout);

				#ifdef DEBUG
					printf("retryCount = %d\n", thisEnv->retryCount);
				#endif
	
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
						debug("key Master: give up", pthread_self());
					}
					else
					{
						debug("key Master: timeout, retry", pthread_self());
					}
					
				}
				// error occured
				else if(selectRC < 0)
				{
					// error
					printf("%s - ", strerror(errno));
					debug("key Master select() ERROR", pthread_self());
				}
				// data received
				else
				{
					// fd ready
					debug("key Master: got sync res", pthread_self());
					read(thisEnv->Read2MasterPipe[READEND], &buffer[0], sizeof(buffer));

					if(1)	// check package if valid response
					{

					}
					else	// no valid SYNC RESPONSE - retry
					{

					}
				}
			break;

			// looks like this node is alone - reset global counter and choose global key randomly from key space
			case CHOOSE_KEY:
				thisEnv->globalCount = 0;
				thisEnv->state = READY;
				if(RAND_bytes(thisEnv->globalKey, GKSIZE) != 1)
				{
					debug("key Master: KEY SELECT FAILED!", pthread_self());
				}
				printKey(thisEnv->globalKey, GKSIZE);
				debug("key Master: set globalCtr = 0, choose new key", pthread_self());
			break;

			// node is ready to process datagrams
			case READY:
				debug("key Master READY, waiting for data", pthread_self());
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
	int (*secRecvThreadfPtr)(void *);
	secRecvThreadfPtr = &secReceive;

	int (*secSendThreadfPtr)(void *);
	secSendThreadfPtr = &secSend;

	struct threadEnvSec_t *threadEnvSec = (struct threadEnvSec_t *)threadEnv;

	pthread_t secRecvThread, secSendThread;
	printf("%u sock = %s id = %d\n", (unsigned)pthread_self(),threadEnvSec->socket, threadEnvSec->id);

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
