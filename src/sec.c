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

void updateGlobalCount(void *env)
{
	threadEnvSec_t *thisEnv = (threadEnvSec_t *)env;
	uint32_t buf;
	uint8_t i=0;
	buf = thisEnv->secGlobalCountInt;
	for(i=0; i<GLOBALCOUNTSIZE;i++)
	{
		thisEnv->secGlobalCount[i] = buf % 256;
		buf = buf / 256;
	}
		#ifdef DEBUG
			printf("SEC%d: countInt = %d, str = ",  thisEnv->id, thisEnv->secGlobalCountInt);
			printf("%02x ", thisEnv->secGlobalCount[i]);
		printf("\n");
		#endif

	thisEnv->secGlobalCountInt++;
}

/*
	FIXME:	use window for check, now sender-recp must be perfectly syncronized
*/
int checkFreshness(void *env, uint8_t *buffer)
{
	uint8_t i=0;
	threadEnvSec_t *thisEnv = (threadEnvSec_t *)env;
	time2Str(env, secBufferTime[thisEnv->id]);
	for(i=0;i<4;i++)
	{
		printf("SEC%d: comparing %02x - %02x\n", thisEnv->id, secBufferTime[thisEnv->id][i], buffer[i]);
		if(secBufferTime[thisEnv->id][i] != buffer[i])
		{
			return FALSE;
		}
	}
	return TRUE;
}
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
			secBufferMAC[thisEnv->id][0] = 0x80;				// set correct frame type(std frame)
			secBufferMAC[thisEnv->id][1] = (1<<4) | (thisEnv->id);		// SRC  = my addr 
			secBufferMAC[thisEnv->id][2] = thisEnv->addrInt;		// SRC  = my addr
			secBufferMAC[thisEnv->id][3] = 0x00;				// DEST = broadcast
			secBufferMAC[thisEnv->id][4] = 0x00;				// DEST = broadcast
			secBufferMAC[thisEnv->id][5] = 0x88;				// set address type + len( byte payload), 
											// but IGNORE TTL!!
			secBufferMAC[thisEnv->id][6] = syncReq;				// SEC HEADER	=~	ACPI
			secBufferMAC[thisEnv->id][7] = secBufferTime[thisEnv->id][0];	// TIME
			secBufferMAC[thisEnv->id][8] = secBufferTime[thisEnv->id][1];	// ...
			secBufferMAC[thisEnv->id][9] = secBufferTime[thisEnv->id][2];	// ...
			secBufferMAC[thisEnv->id][10] = secBufferTime[thisEnv->id][3];	// TIME 

			len = 11;		

			i = generateHMAC(secBufferMAC[thisEnv->id], len, &sigHMAC[thisEnv->id], &thisEnv->slen, thisEnv->skey);
			assert(i == 0);
			if(i != 0)
			{
				printf("SEC%d: FATAL, generateMAC() failed, exit\n", thisEnv->id);
				exit(-1);
			}
		//	#ifdef DEBUG
		//		print_it("HMAC / SYNC", sigHMAC[thisEnv->id], DIGESTSIZE/8);
		//	#endif

			// assemble sync request message
			MSGBUF_SEC2WR[thisEnv->id].frame.buf[0] = secBufferMAC[thisEnv->id][6];
			MSGBUF_SEC2WR[thisEnv->id].frame.buf[1] = secBufferMAC[thisEnv->id][7];
			MSGBUF_SEC2WR[thisEnv->id].frame.buf[2] = secBufferMAC[thisEnv->id][8];
			MSGBUF_SEC2WR[thisEnv->id].frame.buf[3] = secBufferMAC[thisEnv->id][9];
			MSGBUF_SEC2WR[thisEnv->id].frame.buf[4] = secBufferMAC[thisEnv->id][10];
			// ... and append MAC
			for(i = 0; i < MACSIZE; i++)
			{
				MSGBUF_SEC2WR[thisEnv->id].frame.buf[i+5] = sigHMAC[thisEnv->id][i];
			}
		
			//MSGBUF_SEC2WR[thisEnv->id].frame.buf[len-4+MACSIZE] = '\0';
			MSGBUF_SEC2WR[thisEnv->id].frame.len = (9);

			// call write thread directly from here
			secWRnew(MSGBUF_SEC2WR[thisEnv->id].frame.buf, MSGBUF_SEC2WR[thisEnv->id].frame.len, syncReq, env);
		break;
		case syncRes:
			secBufferMAC[thisEnv->id][0] = 0x80;				// set correct frame type(std frame)
			secBufferMAC[thisEnv->id][1] = (1<<4) | (thisEnv->id);		// SRC  = my addr 
			secBufferMAC[thisEnv->id][2] = thisEnv->addrInt;		// SRC  = my addr
			secBufferMAC[thisEnv->id][3] = 0x00;				// DEST = broadcast	FIXME - set correct address
			secBufferMAC[thisEnv->id][4] = 0x00;				// DEST = broadcast	FIXME - set correct address
			secBufferMAC[thisEnv->id][5] = 0x88;				// set address type + len( byte payload), 
											// but IGNORE TTL!!
			// assemble the payload
			secBufferMAC[thisEnv->id][6] = syncRes;				// SEC HEADER	=~	ACPI
			secBufferMAC[thisEnv->id][7] = thisEnv->secGlobalCount[0];	// global Counter
			secBufferMAC[thisEnv->id][8] = thisEnv->secGlobalCount[1];	// ...
			secBufferMAC[thisEnv->id][9] = thisEnv->secGlobalCount[2];	// ...
			secBufferMAC[thisEnv->id][10] = thisEnv->secGlobalCount[3];	// global counter
			secBufferMAC[thisEnv->id][11] = secBufferTime[thisEnv->id][0];	// TIME
			secBufferMAC[thisEnv->id][12] = secBufferTime[thisEnv->id][1];	// ...
			secBufferMAC[thisEnv->id][13] = secBufferTime[thisEnv->id][2];	// ...
			secBufferMAC[thisEnv->id][14] = secBufferTime[thisEnv->id][3];	// TIME 
			
			len = 15;

			i = generateHMAC(secBufferMAC[thisEnv->id], len, &sigHMAC[thisEnv->id], &thisEnv->slen, thisEnv->skey);
			assert(i == 0);
			if(i != 0)
			{
				printf("SEC%d: FATAL, generateMAC() failed, exit\n", thisEnv->id);
				exit(-1);
			}

			// now that we got the MAC, prepare the actual payload
			MSGBUF_SEC2WR[thisEnv->id].frame.buf[0] = secBufferMAC[thisEnv->id][6];
			// append global counter
			MSGBUF_SEC2WR[thisEnv->id].frame.buf[1] = secBufferMAC[thisEnv->id][7];
			MSGBUF_SEC2WR[thisEnv->id].frame.buf[2] = secBufferMAC[thisEnv->id][8];
			MSGBUF_SEC2WR[thisEnv->id].frame.buf[3] = secBufferMAC[thisEnv->id][9];
			MSGBUF_SEC2WR[thisEnv->id].frame.buf[4] = secBufferMAC[thisEnv->id][10];
			// append actual time
			MSGBUF_SEC2WR[thisEnv->id].frame.buf[5] = secBufferMAC[thisEnv->id][11];
			MSGBUF_SEC2WR[thisEnv->id].frame.buf[6] = secBufferMAC[thisEnv->id][12];
			MSGBUF_SEC2WR[thisEnv->id].frame.buf[7] = secBufferMAC[thisEnv->id][13];
			MSGBUF_SEC2WR[thisEnv->id].frame.buf[8] = secBufferMAC[thisEnv->id][14];
			// append MAC
			for(i = 0; i < MACSIZE; i++)
			{
				MSGBUF_SEC2WR[thisEnv->id].frame.buf[i+9] = sigHMAC[thisEnv->id][i];
			}
			MSGBUF_SEC2WR[thisEnv->id].frame.len = (13);

			// call write thread directly from here
			secWRnew(MSGBUF_SEC2WR[thisEnv->id].frame.buf, MSGBUF_SEC2WR[thisEnv->id].frame.len, syncReq, env);

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

	switch(type)	
	{
		case syncReq:		// this is a broadcast message

			//if ((EIBOpenT_Broadcast(thisEnv->secFDWR, 1)) == -1)
			if ((EIBOpenT_Broadcast(thisEnv->secFDWR, 0)) == -1)
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
				printf("\tSEC%d-WR: SEND OK to FD @ %p\n", thisEnv->id, thisEnv->secFDWR);
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
		writes received data to main-sec-thread over pipe
*/
void secRD(void *env)
{
	threadEnvSec_t *thisEnv = (threadEnvSec_t *)env;
	knxPacket tmp;
	uint8_t i = 0, rc = 0;

	thisEnv->secFDRD = EIBSocketURL(thisEnv->socketPath);
	if (!thisEnv->secFDRD)
	{
		printf("\tSEC%d-RD: EIBSocketURL() failed: %s\n\n", thisEnv->id, thisEnv->socketPath);
		exit(-1);		// FIXME: retries...!?
	}
	else
	{
		printf("\tSEC%d-RD: EIBSocketURL() OK %s, using addr %d\n", thisEnv->id, thisEnv->socketPath, thisEnv->addrInt);
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
			printf("\tSEC%d-RD: EIBGetVBusMonitorPacket() FAILED", thisEnv->id);
		}
		else
		{
			if(rc == 1)
			{
				if(thisEnv->secRDbuf[0] == 0xCC)
					printf("\tSEC%d-RD: ACK\n", thisEnv->id);
				else
					printf("\tSEC%d-RD: unknown 1 byte", thisEnv->id);
			}
			// this should NOT happen
			else if(rc < (10 + MACSIZE))
			{
				printf("\tSEC%d-RD: ** SHORT ** - RAW: ", thisEnv->id);
				for(i=0; i<rc;i++)
				{
					printf("%02x ", thisEnv->secRDbuf[i]);
				}
				printf(" = %d bytes total\n", rc);
			}
			else
			{
				decodeFrame(thisEnv->secRDbuf, &tmp);
				if(tmp.srcDev == thisEnv->addrInt)
				{
					#ifdef DEBUG
						printf("\tSEC%d-RD: ignore own broadcast message from devAddr %d\n ", thisEnv->id, thisEnv->addrInt);
					#endif
				}
				else
				{
					#ifdef DEBUG
						printf("\tSEC%d-RD: RAW: ", thisEnv->id);
						for(i=0; i<rc;i++)
						{
							printf("%02x ", thisEnv->secRDbuf[i]);
						}
						printf(" = %d bytes total\n", rc);
					#endif
					if(tmp.type == stdFrame)
					{
						thisEnv->secRDbuf[0] &= 0x80;	// zero-out repeat flag + priority
						thisEnv->secRDbuf[5] &= 0x8F;	// zero-out TTL, which gets changed by routers
						//	check the MAC, use the first x bytes: header + payload (withouth MAC, without FCK)
						i = verifyHMAC(thisEnv->secRDbuf, (rc - MACSIZE - 1), &thisEnv->secRDbuf[rc-5], MACSIZE, thisEnv->skey);

						if(i != 0)
						{
							printf("SEC%d: verifyHMAC() failed, possible attack?\n", thisEnv->id);
							for(i=0; i<rc;i++)
							{
								printf("%02x ", thisEnv->secRDbuf[i]);
							}
							printf(" / %d bytes total\n", rc);
						}
						else
						{
							// MAC is OK - process message
							write(thisEnv->RD2MasterPipe[WRITEEND], &thisEnv->secRDbuf[6], rc - 6 - MACSIZE - 1);
						}
					}
					else
					{	// FIXME: 
					}
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
	int selectRC = 0, rc = 0;
	struct timeval syncTimeout;

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
				
				FD_ZERO(&thisEnv->set);
				FD_SET(thisEnv->RD2MasterPipe[READEND], &thisEnv->set);
			
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

				selectRC = select(FD_SETSIZE, &thisEnv->set, NULL, NULL, &syncTimeout);

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
					read(thisEnv->RD2MasterPipe[READEND], &buffer[0], sizeof(buffer));
/*
					while()
					{

					}
*/
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

				/*
					FIXME: increase security by choosing from random insteat setting to zero....?!
				*/
				thisEnv->secGlobalCountInt = 0x01020304;
				updateGlobalCount(thisEnv);
				thisEnv->state = STATE_READY;

			break;

			// node is ready to process datagrams
			case STATE_READY:
				#ifdef DEBUG
					printf("SEC%d: key master ready\n", thisEnv->id);
				#endif
				while(1)
				{
					read(thisEnv->RD2MasterPipe[READEND], &buffer[0], 1);	// FIXME - must be non-blocking!!
					if(buffer[0] == syncReq)
					{
						#ifdef DEBUG
							printf("SEC%d: got syncReq\n", thisEnv->id);
						#endif
							rc = read(thisEnv->RD2MasterPipe[READEND], &buffer[0], 4);
							if(rc == 4)
							{
								rc = checkFreshness(thisEnv, &buffer[0]);
								if(rc)
								{
									printf("SEC%d: got fresh syncReq\n", thisEnv->id);
									preparePacket(env, syncRes);
								}
								else
								{
									printf("SEC%d: outdated syncReq\n", thisEnv->id);	
								}
							}
							else
							{
								printf("SEC%d: malformed syncReq\n", thisEnv->id);
							}
						
					}
					else
					{
						#ifdef DEBUG
							printf("SEC%d: got unknown input\n", thisEnv->id);
						#endif

					}
				}
			break;

			default:
			assert(0);
		}
	};
				
	printf("key Master EXIT\n");
}

/*
	entry point function from main thread - called 2 times
		uses secWRnew to write data to bus
		communicates with read-thread secRD() over pipe
*/
int initSec(void *threadEnv)
{
	threadEnvSec_t *thisEnv = (threadEnvSec_t *)threadEnv;

	#ifdef DEBUG
		printf("SEC%d: sock = %s, thread# = %u\n", thisEnv->id, thisEnv->socketPath, (unsigned)pthread_self());
	#endif

	#ifdef DEBUG
		printf("SEC%d: / pipFD: %d <- %d\n", thisEnv->id, thisEnv->RD2MasterPipe[READEND], thisEnv->RD2MasterPipe[WRITEEND]);
	#endif

	hmacInit(&thisEnv->skey, &thisEnv->vkey, &thisEnv->slen, &thisEnv->vlen);
	keyInit(threadEnv);
	
	printf("goiing home\n");
	return 0;
}
