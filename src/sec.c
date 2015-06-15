#include "globals.h"

/*
	harald glanzer, 0727156
	harald.glanzer@gmail.com

	needs 3 running eib daemones:
		eibd --listen-local=/tmp/eib.clr  -t31 -e 1.0.<addr>  tpuarts:/dev/tty<DEV-cleartext>
		eibd --listen-local=/tmp/eib.sec1 -t31 -e 1.1.<addr> tpuarts:/dev/tty<DEV-secured-1>
		eibd --listen-local=/tmp/eib.sec2 -t31 -e 1.2.<addr>  tpuarts:/dev/tty<DEv-secured-2>
*/

extern pthread_mutex_t SecMutexWr[SECLINES];
extern pthread_cond_t  SecCondWr[SECLINES];

extern byte secBufferMAC[SECLINES][BUFSIZE];
extern byte secBufferTime[SECLINES][BUFSIZE];

extern byte *sigHMAC[SECLINES];
extern struct msgbuf_t MSGBUF_SEC2WR[SECLINES];

//struct threadEnvSec_t *thisEnv[SECLINES];

//FIXME: this is very insecure.
//	maybe better:
//	read from file to memory, securely delete file, delete buffer after initial phase...?
uint8_t PSK[PSKSIZE] =	"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x30\x31";


/*
	harald glanzer
	secure line thread
*/

/*
	convert unix-time time_t to 4 digit hex string
*/
void time2Str(void *env, byte *buf)
{
	threadEnvSec_t *thisEnv = (threadEnvSec_t *)env;
	thisEnv->now = time(NULL);
	int i=0;

	for(i=3; i>=0;i=i-1)
	{
		buf[i] = thisEnv->now % 256;
		thisEnv->now = thisEnv->now / 256;
	}
}

void preparePacket(void *env, uint8_t type)
{
	threadEnvSec_t *thisEnv = (threadEnvSec_t *)env;
	uint8_t i = 0, len = 0;
	time2Str(env, secBufferTime[thisEnv->id]);

	MSGBUF_SEC2WR[thisEnv->id].frame.buf[0] = '\0';
	secBufferMAC[thisEnv->id][0] = '\0';

	/*
		prepare...
	*/
	switch(type)
	{
		case syncReq:
		//	secBufferMAC[thisEnv->id][0] = (EIBADDR(1,thisEnv->id, thisEnv->addrInt));		// SRC  = my addr 
			secBufferMAC[thisEnv->id][0] = (1<<4) | (thisEnv->id);		// SRC  = my addr 
			secBufferMAC[thisEnv->id][1] = thisEnv->addrInt;		// SRC  = my addr
			secBufferMAC[thisEnv->id][2] = 0x00;				// DEST = broadcast
			secBufferMAC[thisEnv->id][3] = 0x00;				// DEST = broadcast
			secBufferMAC[thisEnv->id][4] = syncReq;				// SEC HEADER
			secBufferMAC[thisEnv->id][5] = secBufferTime[thisEnv->id][0];	// TIME
			secBufferMAC[thisEnv->id][6] = secBufferTime[thisEnv->id][1];	// ...
			secBufferMAC[thisEnv->id][7] = secBufferTime[thisEnv->id][2];	// ...
			secBufferMAC[thisEnv->id][8] = secBufferTime[thisEnv->id][3];	// TIME 
//			secBufferMAC[thisEnv->id][9] = '\0';				// delimiter 

			len = 9;		

			i = generateHMAC(secBufferMAC[thisEnv->id], len, &sigHMAC[thisEnv->id], &thisEnv->slen, thisEnv->skey);
			//i = generateHMAC(secBufferMAC[thisEnv->id], len, &sigHMAC[thisEnv->id], &slen[thisEnv->id], skey[thisEnv->id]);
			assert(i == 0);
			if(i != 0)
			{
				printf("SEC%d: FATAL, generateMAC() failed, exit\n", thisEnv->id);
				exit(-1);
			}
			#ifdef DEBUG
				print_it("HMAC / SYNC", sigHMAC[thisEnv->id], DIGESTSIZE/8);
			#endif
			// assemble sync request message
			for(i = 0; i <= len - 5; i++)
			{
				MSGBUF_SEC2WR[thisEnv->id].frame.buf[i] = secBufferMAC[thisEnv->id][i+4];
				//printf("%02X ", MSGBUF_SEC2WR[thisEnv->id].buf[i]);
			}
			for(i = 0; i < MACSIZE; i++)
			{
				MSGBUF_SEC2WR[thisEnv->id].frame.buf[i+(len-4)] = sigHMAC[thisEnv->id][i];
				//printf("%02X ", MSGBUF_SEC2WR[thisEnv->id].buf[i]);
			}
		
			MSGBUF_SEC2WR[thisEnv->id].frame.buf[len-4+MACSIZE] = '\0';
			MSGBUF_SEC2WR[thisEnv->id].frame.len = (len-4+MACSIZE);

			// call write thread directly from here
			secWRnew(MSGBUF_SEC2WR[thisEnv->id].frame.buf, MSGBUF_SEC2WR[thisEnv->id].frame.len, syncReq, env);
		break;
		case syncRes:

		break;
		default:
			printf("prepare(): THIS SHOULD NOT HAPPEN, exit");
			exit(-1);
	}
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

	OLD DISABLED	- now handled by sec - master thread
int secWR(void *env)
{
	threadEnvSec_t *thisEnv = (threadEnvSec_t *)env;
	printf("\tSEC%d-WR: %s with FD @ %p / %u\n", thisEnv->id, thisEnv->socketPath, thisEnv->secFDWR, (unsigned)pthread_self());

	thisEnv->MSGIDsecWR = msgget(MSGKEY_SEC2WR, MSG_PERM | IPC_CREAT);
	if(thisEnv->MSGIDsecWR == -1)
	{
		printf("SEC%d-WR: message queue msgget() failed\n", thisEnv->id);
		exit(-1);
	}
	while(1)
	{
		//wait for new data to send over secure line
		//pthread_mutex_lock(&SecMutexWr[thisEnv->id]);
		//pthread_cond_wait(&SecCondWr[thisEnv->id], &SecMutexWr[thisEnv->id]);
		//pthread_mutex_unlock(&SecMutexWr[thisEnv->id]);
		//msgrcv(thisEnv->MSGIDsecWR, &MSGBUF_SEC2WR[thisEnv->id], sizeof(MSGBUF_SEC2WR[thisEnv->id]) - sizeof(long), 0, 0);

		switch(MSGBUF_SEC2WR[thisEnv->id].frame.type)	
		{
			case syncReq:		// this is a broadcast message

				thisEnv->secFDWR = EIBSocketURL(thisEnv->socketPath);
				#ifdef DEBUG
					printf("\tSEC%d-WR: %s with FD @ %p / %u\n", thisEnv->id, thisEnv->socketPath, thisEnv->secFDWR, (unsigned)pthread_self());
				#endif
				if (!(thisEnv->secFDWR))
				{
					printf("SEC%d-WR: EIBSocketURL() with FD @ %p failed\n\n", thisEnv->id, thisEnv->secFDWR);
					exit(-1);
				}

				if ((EIBOpenT_Broadcast(thisEnv->secFDWR, 0)) == -1)
				{
					printf("SEC%d-WR: EIBOpenT_Broadcast() failed\n\n", thisEnv->id);
					exit(-1);
				}
				if (EIBSendAPDU(thisEnv->secFDWR, MSGBUF_SEC2WR[thisEnv->id].frame.len, (unsigned char *)MSGBUF_SEC2WR[thisEnv->id].frame.buf) == -1)
				{
					printf("EIBOpenSendTPDU() failed\n\n");
        				exit(-1);
				}
				#ifdef DEBUG
					printf("\tSEC%d-WR: SEND OK to FD @ %p\n", thisEnv->id, thisEnv->secFDWR);
				#endif
				EIBClose(thisEnv->secFDWR);
				//EIBClose(thisEnv->secFDWR);

			break;
			case syncRes:

			break;
		}
		#ifdef DEBUG
			printf("\tSEC%d-WR: new MSQ data: ", thisEnv->id);
			thisEnv->tmpsecWR = 0;
			while(MSGBUF_SEC2WR[thisEnv->id].frame.buf[thisEnv->tmpsecWR] != '\0')
			{
				printf("%02x ", MSGBUF_SEC2WR[thisEnv->id].frame.buf[thisEnv->tmpsecWR]);
				thisEnv->tmpsecWR++;
			}

			printf("\n");
		#endif
	}
	return 0;
}
*/
int secWRnew(char *buf, uint8_t len, uint8_t type, void *env)
{
	threadEnvSec_t *thisEnv = (threadEnvSec_t *)env;

	thisEnv->secFDWR = EIBSocketURL(thisEnv->socketPath);
	if (!thisEnv->secFDWR)
	{
		printf("  SEC%d-WR: EIBSocketURL() failed: %s\n\n", thisEnv->id, thisEnv->socketPath);
		exit(-1);
	}
	else
	{
		printf("  SEC%d-WR: EIBSocketURL() success %s\n", thisEnv->id, thisEnv->socketPath);
	}

	switch(type)	
	{
		case syncReq:		// this is a broadcast message


			if ((EIBOpenT_Broadcast(thisEnv->secFDWR, 1)) == -1)
			//if ((EIBOpenT_Broadcast(thisEnv->secFDWR, 0)) == -1)
			{
				printf("SEC%d-WR: EIBOpenT_Broadcast() failed\n\n", thisEnv->id);
				exit(-1);
			}
			if (EIBSendAPDU(thisEnv->secFDWR, len, (const uint8_t *)buf) == -1)
			{
				printf("EIBOpenSendTPDU() failed\n\n");
       				exit(-1);
			}
			#ifdef DEBUG
				printf("  SEC%d-WR: SEND OK to FD @ %p\n", thisEnv->id, thisEnv->secFDWR);
			#endif

		break;
		case syncRes:

		break;
		default:
			printf("default: ARGL\n");
		
	}
	EIBClose(thisEnv->secFDWR);
	return 0;
}

/*
	sec RECEIVE thread
*/
void secRD(void *env)
{
	threadEnvSec_t *thisEnv = (threadEnvSec_t *)env;
	knxPacket tmp;
	uint8_t i = 0, rc = 0;

	thisEnv->secFDRD = EIBSocketURL(thisEnv->socketPath);
	if (!thisEnv->secFDRD)
	{
		printf("  SEC%d-RD: EIBSocketURL() failed: %s\n\n", thisEnv->id, thisEnv->socketPath);
		exit(-1);
	}
	else
	{
		printf("  SEC%d-RD: EIBSocketURL() success %s\n, listening with own dev addr = %d", thisEnv->id, thisEnv->socketPath, thisEnv->addrInt);
	}

	if (EIBOpenVBusmonitor(thisEnv->secFDRD) == -1)
	{
		printf("\tSEC%d-RD: EIBOpenBusmonitor() failed\n\n", thisEnv->id);
	       	exit(-1);
	}

	while(1)
	{
		#ifdef DEBUG
			printf("\tSEC%d-RD: READY\n", thisEnv->id);
		#endif
		rc = EIBGetBusmonitorPacket(thisEnv->secFDRD, sizeof(thisEnv->secRDbuf), thisEnv->secRDbuf);
		if(rc == -1)
		{
			printf("\tSEC%d: EIBGetVBusMonitorPacket() FAILED", thisEnv->id);
		}
		else
		{
			decodeFrame(thisEnv->secRDbuf, &tmp);
			if(tmp.srcDev == thisEnv->addrInt)
			{
				#ifdef DEBUG
					printf("\tSEC%d: ignore own broadcast message from dev %d\n ", thisEnv->id, thisEnv->addrInt);
				#endif
			}
			else
			{
				#ifdef DEBUG
					printf("\tSEC%d: RAW: ", thisEnv->id);
					for(i=0; i<rc;i++)
					{
						printf("%02x ", thisEnv->secRDbuf[i]);
					}
					printf(" = %d bytes total\n", rc);
				#endif
				if(tmp.type == stdFrame)
				{
					i = verifyHMAC(thisEnv->secRDbuf, 9, &thisEnv->secRDbuf[rc-4], MACSIZE, thisEnv->skey);
				}
				else
				{
		// FIXME: 
				}
				assert(i == 0);
				if(i != 0)
				{
					printf("SEC%d: FATAL, generateMAC() failed, exit\n", thisEnv->id);
					exit(-1);
				}
			}
		}
	}
	EIBClose(thisEnv->secFDRD);	
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
	threadEnvSec_t *thisEnv = (threadEnvSec_t *)env;
	uint8_t buffer[BUFSIZE]; 

	thisEnv->state = STATE_INIT;

	/*	
		main state machine for SEC - master thread
		talks to secRD() and secWR()
	*/
	while(1)
	{
		switch(thisEnv->state)
		{
			case STATE_INIT:
				#ifdef DEBUG
					printf("SEC%d: INIT\n", thisEnv->id);
				#endif

/*	
				thisEnv->MSGIDsecMASTER = msgget(MSGKEY_SEC2WR, MSG_PERM);
				if(thisEnv->MSGIDsecMASTER == -1)
				{
					printf("SEC%d-MA: message queue msgget() failed: %s\n", thisEnv->id, strerror(errno));
					exit(-1);
				}
*/
				thisEnv->retryCount = 0;
				FD_ZERO(&set);
				FD_SET(thisEnv->Read2MasterPipe[READEND], &set);
			
				thisEnv->state = STATE_SYNC_REQ;
			break;
			case STATE_SYNC_REQ:
				#ifdef DEBUG
					printf("SEC%d: sending %d. sync req\n", thisEnv->id, thisEnv->retryCount);
				#endif
				// prepare MAC for sync request message	
				preparePacket(env, syncReq);
//MSGBUF_SEC2WR[thisEnv->id].mtype = MSG_TYPE;
//MSGBUF_SEC2WR[thisEnv->id].frame.type = syncReq;
//msgsnd(thisEnv->MSGIDsecMASTER, &MSGBUF_SEC2WR[thisEnv->id], sizeof(MSGBUF_SEC2WR[thisEnv->id]) - sizeof(long), 0);

				thisEnv->state = STATE_SYNC_WAIT_RESP;
			break;
			case STATE_SYNC_WAIT_RESP:
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
					thisEnv->state = STATE_SYNC_REQ;
		
					/*
						seems like there is no other device reachable / online(on this SECline)
							* reset globalCounter
					*/
					if(thisEnv->retryCount == SYNC_RETRIES)
					{
						thisEnv->state = STATE_RESET_CTR;
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
					thisEnv->state = STATE_READY;
				}
			break;

			// looks like this node is alone - reset global counter
			// FIXME:	maybe choose random counter?
			//		or additionally use time() information ?
			case STATE_RESET_CTR:
				#ifdef DEBUG
					printf("SEC%d: RESET_CTR\n", thisEnv->id);
				#endif
				thisEnv->globalCount = 0;
				thisEnv->state = STATE_READY;

			break;

			// node is ready to process datagrams
			case STATE_READY:
				#ifdef DEBUG
					printf("SEC%d: key master ready\n", thisEnv->id);
				#endif
				sleep(100);
			break;

			default:
			assert(0);
		}
	};
				
	printf("key Master EXIT\n");
}

/*
	entry point function from main thread - called 2 times
	generates 1 RECEIVE and 1 SEND thread
*/
int initSec(void *threadEnv)
{
	threadEnvSec_t *thisEnv = (threadEnvSec_t *)threadEnv;

	#ifdef DEBUG
		printf("SEC%d: sock = %s, thread# = %u\n", thisEnv->id, thisEnv->socketPath, (unsigned)pthread_self());
	#endif
	/*
		pipe is used for communication: secRead -> secKeymaster
		with pipes select() with timeouts can be used!
	*/
	if(pipe(thisEnv->Read2MasterPipe) == -1)
	{
		printf("pipe() failed, exit\n");
		exit(-1);
	}

	#ifdef DEBUG
		printf("SEC%d: / pipFD: %d <- %d\n", thisEnv->id, thisEnv->Read2MasterPipe[READEND], thisEnv->Read2MasterPipe[WRITEEND]);
	#endif

	hmacInit(&thisEnv->skey, &thisEnv->vkey, &thisEnv->slen, &thisEnv->vlen);
	keyInit(threadEnv);
	
	printf("goiing home\n");
	return 0;
}
