#include "globals.h"

extern pthread_mutex_t clr2SecMutexWr[SECLINES];
extern pthread_cond_t  clr2SecCondWr[SECLINES];

// for communication secXRd <--> secXKeymaster
extern pthread_mutex_t secMutexInternal[SECLINES];
extern pthread_cond_t  secCondInternal[SECLINES];

extern int newData4Sec;
extern int newData4Cls;

uint8_t	secINIT = 0;

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
		-> authenticated response to syncronize requester
	|---------------------------------------------------------------------------------------------------------------------------------
	|	 |        |	   	    |		     |	      |	       |			|			|	 |
	|   CTRL |CTRL-EXT| Source Address  | DestinationAddr| LENGTH |  SECH  | Global Sequence Counter|      MAC (k_PSK)	|   FC   |  
	|	 | 	  | 	   	    |		     |	      |	       |	4-6 Byte	|	4-6 Byte	|	 |
	|---------------------------------------------------------------------------------------------------------------------------------

----------------------------------------------------------------------------------------------------------------------------------------------------
	
	SEC - join REQUEST
		-> request to obtain the actual, globally used key
	|---------------------------------------------------------------------------------------------------------------------------------
	|	 |        |	   	    |		     |	      |	       |			|			|	 |
	|   CTRL |CTRL-EXT| Source Address  | DestinationAddr| LENGTH |  SECH  | Global Sequence Counter|      MAC (k_PSK)	|   FC   |  
	|	 | 	  | 	   	    |		     |	      |	       |	4-6 Byte	|	4-6 Byte	|	 |
	|---------------------------------------------------------------------------------------------------------------------------------

	SEC - join RESPONSE
		-> response to submit the actual, globally used key to requester
	|---------------------------------------------------------------------------------------------------------------------------------
	|	 |        |	   	    |		     |	      |	       |			|			|	 |
	|   CTRL |CTRL-EXT| Source Address  | DestinationAddr| LENGTH |  SECH  | Global Sequence Counter|      MAC (k_PSK)	|   FC   |  
	|	 | 	  | 	   	    |		     |	      |	       |	4-6 Byte	|	4-6 Byte	|	 |
	|---------------------------------------------------------------------------------------------------------------------------------

----------------------------------------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------------------
*/

EIBConnection *secFD[SECLINES];

/*
	harald glanzer
	secure line thread
*/

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
int secReceive(void *env)
{
	struct threadEnvSec_t *thisEnv = (struct threadEnvSec_t *)env;
	#ifdef DEBUG
		printf("SEC%d , waiting for data from EIBD\n", thisEnv->id);
	#endif
	while(1)
	{
		debug("SEC: blocking(EIBD)", pthread_self());
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
	int i = 0, selectRC = 0;
	struct timeval syncTimeout;
	struct timeval joinTimeout;

	fd_set set;
	struct threadEnvSec_t *thisEnv = (struct threadEnvSec_t *)env;
	uint8_t buffer[BUFSIZE]; 

	thisEnv->state = INIT;
	thisEnv->retryCount = 0;

	/*	
		1a) send sync request		(cleartext)
		1b) wait for sync response	-> obtain global counter(auth)

		2a) send join request		(auth)
		2b) wait for join response	-> obtain global key(auth+enc)

	*/
	FD_ZERO(&set);
	FD_SET(thisEnv->Read2MasterPipe[READEND], &set);

	while(1)
	{
		switch(thisEnv->state)
		{
			case INIT:

				syncTimeout.tv_sec = SYNCTIMEOUT_SEC;
				syncTimeout.tv_usec = 0;	

				debug("key Master: select() / sending sync req", pthread_self());
		//		pthread_mutex_lock();

				// copy data to buffer...

				// and signal it to SEND thread
		//		pthread_signal();
		//		pthread_mutex_unlock();

				selectRC = select(FD_SETSIZE, &set, NULL, NULL, &syncTimeout);
				thisEnv->retryCount++;

				#ifdef DEBUG
					printf("retryCount = %d", thisEnv->retryCount);
				#endif
	
				if(selectRC == 0)
				{
					// timeout
					debug("key Master JOIN timeout, retry", pthread_self());
		
					/*
						seems like there is no other device reachable / online(on this SECline)
							* reset globalCounter
							* choose new global key
							* generate DH parameters	
					*/
					if(thisEnv->retryCount == SYNC_RETRIES)
					{
						debug("max retries / setting global counter = 0x00", pthread_self());
					
					}
					
				}
				else if(selectRC < 0)
				{
					// error
					printf("%s - ", strerror(errno));
					debug("key Master select() ERROR", pthread_self());
				}
				else
				{
					// fd ready
					debug("key Master init, got sync res", pthread_self());
					read(thisEnv->Read2MasterPipe[READEND], &buffer[0], sizeof(buffer));

					if(1)	// check package if valid response
					{

					}
					else	// no valid SYNC RESPONSE - retry
					{

					}
				}
		
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
	printf("this is sec %u, with args sock = %s id = %d\n", (unsigned)pthread_self(),threadEnvSec->socket, threadEnvSec->id);

	

	if((pthread_create(&secRecvThread, NULL, (void *)secRecvThreadfPtr, (void *)threadEnvSec)) != 0)
	{
		printf("sec RECEIVE Thread init failed, exit\n");
		return -1;
	}
	if((pthread_create(&secSendThread, NULL, (void *)secSendThreadfPtr, (void *)threadEnvSec)) != 0)
	//if((pthread_create(&secSendThread, NULL, (void *)secSendThreadfPtr, &threadEnvSec->id)) != 0)
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
