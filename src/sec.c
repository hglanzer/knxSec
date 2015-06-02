#include "globals.h"

extern pthread_mutex_t SecMutexWr[SECLINES];
extern pthread_cond_t  SecCondWr[SECLINES];

byte secBufferMAC[SECLINES][BUFSIZE];
byte secBufferWr[SECLINES][BUFSIZE];
byte secBufferTime[SECLINES][BUFSIZE];

uint8_t	secINIT = 0;

EVP_PKEY *skey[SECLINES];
EVP_PKEY *vkey[SECLINES];
size_t slen[SECLINES];
size_t vlen[SECLINES];
byte *sigHMAC[SECLINES];
struct msgbuf_t MSGBUF_SEC2WR[SECLINES];

//FIXME: this is very insecure.
//	maybe better:
//	read from file to memory, securely delete file, delete buffer after initial phase...?
uint8_t PSK[PSKSIZE] =	"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x30\x31";

EIBConnection *secFD[SECLINES];

/*
	harald glanzer
	secure line thread
*/

/*
	convert unix-time time_t to 4 digit hex string
*/
void time2Str(byte *buf)
{
	time_t now = time(NULL);
	int i=0;

	for(i=3; i>=0;i=i-1)
	{
		buf[i] = now % 256;
		now = now / 256;
	}
}

void prepareSyncReq(void *env)
{
	struct threadEnvSec_t *thisEnv = (struct threadEnvSec_t *)env;
	time2Str(secBufferTime[thisEnv->id]);

	secBufferMAC[thisEnv->id][0] = 0;				// DEST = broadcast
	secBufferMAC[thisEnv->id][1] = 0;				// DEST = broadcast
	secBufferMAC[thisEnv->id][2] = (1<<4) | (thisEnv->id);		// SRC  = my addr FIXME
	secBufferMAC[thisEnv->id][3] = thisEnv->addrInt;		// SRC  = my addr FIXME
	secBufferMAC[thisEnv->id][4] = syncReq;				// SEC HEADER
	secBufferMAC[thisEnv->id][5] = secBufferTime[thisEnv->id][0];	// TIME
	secBufferMAC[thisEnv->id][6] = secBufferTime[thisEnv->id][1];	// ...
	secBufferMAC[thisEnv->id][7] = secBufferTime[thisEnv->id][2];	// ...
	secBufferMAC[thisEnv->id][8] = secBufferTime[thisEnv->id][3];	// TIME 
	secBufferMAC[thisEnv->id][9] = '\0';				// delimiter 
}

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
	sec SEND thread
*/
int secWR(void *env)
{
	int rc = 0, i = 0;
	struct threadEnvSec_t *thisEnv = (struct threadEnvSec_t *)env;
	#ifdef DEBUG
		printf("SEC%d: send ready\n", thisEnv->id);
	#endif

	rc = msgget(MSGKEY_SEC2WR, MSG_PERM | IPC_CREAT);
	if(rc == -1)
	{
		printf("SEC%d-WR: message queue msgget() failed\n", thisEnv->id);
		exit(-1);
	}
	#ifdef DEBUG
		printf("SEC%d-WR: msgget() OK\n", thisEnv->id);
	#endif
	while(1)
	{
	/*		USE MSQ with blocking msgrcv() instead

		// wait for new data to send over secure line
		pthread_mutex_lock(&SecMutexWr[thisEnv->id]);
		pthread_cond_wait(&SecCondWr[thisEnv->id], &SecMutexWr[thisEnv->id]);
		#ifdef DEBUG
			printf("SEC%d: sending payload = 0x%X\n", thisEnv->id, secBufferWr[thisEnv->id][0]);
		#endif
		pthread_mutex_unlock(&SecMutexWr[thisEnv->id]);
	*/
		#ifdef DEBUG
			printf("SEC%d-WR: waiting MSQ data\n", thisEnv->id);
		#endif

		msgrcv(rc, &MSGBUF_SEC2WR[thisEnv->id], sizeof(MSGBUF_SEC2WR[thisEnv->id]) - sizeof(long), 0, 0);
		#ifdef DEBUG
			printf("SEC%d-WR: new MSQ data: ", thisEnv->id);
			i = 0;
			while(MSGBUF_SEC2WR[thisEnv->id].buf[i] != '\0')
			{
				printf("%02x ", MSGBUF_SEC2WR[thisEnv->id].buf[i]);
				i++;
			}

			printf("\n");
		#endif
	}
	return 0;
}

/*
	sec RECEIVE thread
							|------> secWR				(msg messages)
							|
	datapath:	secRD	 ---> secMaster --------|
							|
							|------> checkDup ----> clrSend		(knx traffic)
*/
int secRD(void *env)
{
	int rc = 0, i = 0;
	struct msgbuf_t localBuf;
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
				printf("%02x ", (unsigned)thisEnv->localRDBuf);

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
	int selectRC = 0, rc = 0, MSGID_SEC2WR;
	struct timeval syncTimeout;

	fd_set set;
	struct threadEnvSec_t *thisEnv = (struct threadEnvSec_t *)env;
	uint8_t buffer[BUFSIZE]; 

	thisEnv->state = INIT;

	/*	
		main state machine for SEC - master thread
		talks to secRD() and secWR()
	*/
	while(1)
	{
		switch(thisEnv->state)
		{
			case INIT:
				#ifdef DEBUG
					printf("SEC%d: INIT\n", thisEnv->id);
				#endif
	
				MSGID_SEC2WR = msgget(MSGKEY_SEC2WR, MSG_PERM);
				if(rc == -1)
				{
					printf("SEC%d-MA: message queue msgget() failed\n", thisEnv->id);
					exit(-1);
				}
				#ifdef DEBUG
					printf("SEC%d-MA: msgget() OK\n", thisEnv->id);
				#endif

				thisEnv->retryCount = 0;
				FD_ZERO(&set);
				FD_SET(thisEnv->Read2MasterPipe[READEND], &set);
			
				thisEnv->state = SYNC_REQ;
			break;
			case SYNC_REQ:
				#ifdef DEBUG
					printf("SEC%d: sending %d. sync req\n", thisEnv->id, thisEnv->retryCount);
				#endif
				// prepare MAC for sync request message	
				prepareSyncReq(env);
				rc = generateHMAC(secBufferMAC[thisEnv->id], 9, &sigHMAC[thisEnv->id], &slen[thisEnv->id], skey[thisEnv->id]);
				assert(rc == 0);
				if(rc != 0)
				{
					#ifdef DEBUG
						printf("SEC%d: FATAL, generateMAC() failed, exit\n", thisEnv->id);
					#endif
					exit(-1);
				}
				#ifdef DEBUG
					print_it("HMAC / SYNC", sigHMAC[thisEnv->id], DIGESTSIZE/8);
				#endif
				
				// assemble sync request message
				strcpy(MSGBUF_SEC2WR[thisEnv->id].buf, &secBufferMAC[thisEnv->id][4]);

// FIXME: RESTRICT MAC SIZE TO 4 BYTE (use define) - CHECK FOR BUFFER OVERFLOWS!!!! strncat()
				strcat(MSGBUF_SEC2WR[thisEnv->id].buf, sigHMAC[thisEnv->id]);
				MSGBUF_SEC2WR[thisEnv->id].mtype = MSG_TYPE;
				msgsnd(MSGID_SEC2WR, &MSGBUF_SEC2WR[thisEnv->id], sizeof(MSGBUF_SEC2WR[thisEnv->id]) - sizeof(long), 0);
/*
				pthread_mutex_lock(&SecMutexWr[thisEnv->id]);

				// write to buffer				

				pthread_cond_signal(&SecCondWr[thisEnv->id]);
				pthread_mutex_unlock(&SecMutexWr[thisEnv->id]);
*/
				thisEnv->state = SYNC_WAIT_RESP;
			break;
			case SYNC_WAIT_RESP:
				#ifdef DEBUG
					printf("SEC%d: SYNC_WAIT_RESP\n", thisEnv->id);
				#endif
				syncTimeout.tv_sec = SYNCTIMEOUT_SEC;
				syncTimeout.tv_usec = 0;	

				selectRC = select(FD_SETSIZE, &set, NULL, NULL, &syncTimeout);

				// timeout
				if(selectRC == 0)
				{
					thisEnv->retryCount++;
					thisEnv->state = SYNC_REQ;
		
					/*
						seems like there is no other device reachable / online(on this SECline)
							* reset globalCounter
					*/
					if(thisEnv->retryCount == SYNC_RETRIES)
					{
						thisEnv->state = RESET_CTR;
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

					// SAVE COUNTER!		FIXME	
					thisEnv->state = READY;
				}
			break;

			// looks like this node is alone - reset global counter
			// FIXME:	maybe choose random counter?
			//		or additionally use time() information ?
			case RESET_CTR:
				#ifdef DEBUG
					printf("SEC%d: RESET_CTR\n", thisEnv->id);
				#endif
				thisEnv->globalCount = 0;
				thisEnv->state = READY;

			break;

			// node is ready to process datagrams
			case READY:
				#ifdef DEBUG
					printf("SEC%d: key master ready\n", thisEnv->id);
				#endif
				sleep(100);
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

	int (*secRDThreadfPtr)(void *);
	secRDThreadfPtr = &secRD;

	int (*secWRThreadfPtr)(void *);
	secWRThreadfPtr = &secWR;

	struct threadEnvSec_t *threadEnvSec = (struct threadEnvSec_t *)threadEnv;

	pthread_t secRDThread, secWRThread;
	//printf("%u sock = %s id = %d\n", (unsigned)pthread_self(),threadEnvSec->socketPath, threadEnvSec->id);
	#ifdef DEBUG
		printf("SEC%d: sock = %s, thread# = %u", threadEnvSec->id, threadEnvSec->socketPath, (unsigned)pthread_self());
	#endif

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
	if((pthread_create(&secRDThread, NULL, (void *)secRDThreadfPtr, (void *)threadEnvSec)) != 0)
	{
		printf("sec RECEIVE Thread init failed, exit\n");
		exit(-1);
	}

	// create WRITE thread
	if((pthread_create(&secWRThread, NULL, (void *)secWRThreadfPtr, (void *)threadEnvSec)) != 0)
	{
		printf("sec SEND Thread init failed, exit\n");
		exit(-1);
	}

	/*
		pipe is used for communication: secRead -> secKeymaster
		with pipes select() with timeouts can be used!
	*/
	if(pipe(threadEnvSec->Read2MasterPipe) == -1)
	{
		debug("pipe() failed, exit", pthread_self());
		exit(-1);
	}

	#ifdef DEBUG
		printf("SEC%d: / pipFD: %d <- %d\n", threadEnvSec->id, threadEnvSec->Read2MasterPipe[READEND], threadEnvSec->Read2MasterPipe[WRITEEND]);
	#endif

	hmacInit(&skey[thisEnv->id], &vkey[thisEnv->id], &slen[threadEnvSec->id], &vlen[threadEnvSec->id]);
	keyInit(threadEnvSec);
	
	pthread_join(secRDThread, NULL);
	pthread_join(secWRThread, NULL);
	printf("goiing home\n");
	return 0;
}
