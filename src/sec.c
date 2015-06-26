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
	checks if received counter value > globalCtr
		if yes:	inc counter
			decrypts GA with new counter value
			checks if this GW is responsible for the wanted G.A.
*/
int checkGA(void *env, uint8_t *encGA)
{
	threadEnvSec_t *thisEnv = (threadEnvSec_t *)env;

	printf("SEC-GA%d: got GA = %02X %02X\n", thisEnv->id, encGA[0], encGA[1]);
	printf("FIXME: decrypt\n");

	return TRUE;

}
uint8_t saveGlobalCount(void *env, uint8_t *buffer)
{
	threadEnvSec_t *thisEnv = (threadEnvSec_t *)env;
	uint8_t i = 0;
	uint32_t exp = 1, tmp=0;

printf("buffer = ");
	for(i = 0; i<GLOBALCOUNTSIZE;i++)
		printf("%02x ", buffer[i]);

	for(i = GLOBALCOUNTSIZE;i > 0; i=i-1)
	{
		tmp += buffer[i-1] * exp;
		//thisEnv->secGlobalCountInt += buffer[i-1] * exp;
		//thisEnv->secGlobalCount[i-1] = buffer[i-1];
		exp = exp * 256;
	}
	if(tmp > thisEnv->secGlobalCountInt)
	{
		printf("saving FRESH counterInt = %d\n", tmp);
		thisEnv->secGlobalCountInt = tmp;
		for(i = GLOBALCOUNTSIZE;i > 0; i=i-1)
		{
			thisEnv->secGlobalCount[i-1] = buffer[i-1];
		}
		return TRUE;

	}
	else
	{
		printf("discarding OUTDATED counterInt = %d\n", tmp);
		return FALSE;
	}
}
/*
	secGlobalCounter: [0] ... highest digit
			  ...
			  [3] ... lowest digit
*/
void incGlobalCount(void *env)
{
	threadEnvSec_t *thisEnv = (threadEnvSec_t *)env;
	uint32_t buf;
	uint8_t i=0;
	
	thisEnv->secGlobalCountInt++;
	buf = thisEnv->secGlobalCountInt;
	for(i=GLOBALCOUNTSIZE; i>0;i=i-1)
	{
		thisEnv->secGlobalCount[i-1] = buf % 256;
		buf = buf / 256;
	}
	#ifdef DEBUG
		printf("SEC%d: countInt = %d, str = ",  thisEnv->id, thisEnv->secGlobalCountInt);
	for(i=0; i<GLOBALCOUNTSIZE;i++)
			printf("%02x ", thisEnv->secGlobalCount[i]);
		
		printf("\n");
	#endif
}

/*
	FIXME:	use window for check, now sender-recp must be perfectly syncronized
*/
int checkFreshness(void *env, uint8_t *buffer)
{
	long myTime = 0, exp = 1, i=0;
	time_t now = time(NULL);
/*	
	time2Str(env, secBufferTime[thisEnv->id]);
	for(i=0;i<4;i++)
	{
		//printf("SEC%d: comparing %02x - %02x\n", thisEnv->id, secBufferTime[thisEnv->id][i], buffer[i]);
		if(secBufferTime[thisEnv->id][i] != buffer[i])
		{
			return FALSE;
		}
	}
*/
//	convert unix-time time_t to 4 digit hex string
	for(i=4; i>0;i=i-1)
	{
		myTime = myTime + buffer[i-1]*exp;
		exp = exp*256;
	}
	//printf("check_freshness: %ld - %ld = %ld\n", now, myTime, labs(myTime-now));
	if(abs(myTime - now) < 5)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
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

void preparePacket(void *env, uint8_t type, uint8_t *dest, uint8_t *destGA, uint8_t *dhPubKey)
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
			secWRnew(MSGBUF_SEC2WR[thisEnv->id].frame.buf, MSGBUF_SEC2WR[thisEnv->id].frame.len, syncReq, env, NULL);
		break;
		case syncRes:
			secBufferMAC[thisEnv->id][0] = 0x80;				// set correct frame type(std frame)
			secBufferMAC[thisEnv->id][1] = (1<<4) | (thisEnv->id);		// SRC  = my addr 
			secBufferMAC[thisEnv->id][2] = thisEnv->addrInt;		// SRC  = my addr
			secBufferMAC[thisEnv->id][3] = dest[0];				// DEST = src of requester
			secBufferMAC[thisEnv->id][4] = dest[1];				
			secBufferMAC[thisEnv->id][5] = 0x0C;				// set address type + len( byte payload), 
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
			secWRnew(MSGBUF_SEC2WR[thisEnv->id].frame.buf, MSGBUF_SEC2WR[thisEnv->id].frame.len, syncRes, env, dest);

		break;

	/*
			CTRLE: tTTL  xxxx
			       | |     |______	RESERVED, set to ZERO
			       | |____________	TTL / MASK to ignore
			       |______________	0 = point-to-point extended, 1 = group addressed extended

	*/
		default:
			if(type == discReq)
			{
				printf("SEC%d: generating discREQUEST for G.A = %d %d: ", thisEnv->id, destGA[0], destGA[1]);
				// this is a broadcast
				secBufferMAC[thisEnv->id][1] = 0x10;				// set CTRLE		(ignore TTL)
			}
			else if(type == discRes)
			{
				printf("SEC%d: generating discRESPONSE to %d %d for G.A = %d %d: ", thisEnv->id, dest[0], dest[1], destGA[0], destGA[1]);
				// this is unicast
				secBufferMAC[thisEnv->id][1] = 0x00;				// set CTRLE		(ignore TTL)

			}
			else
			{
				printf("prepare(): THIS SHOULD NOT HAPPEN, exit");
				exit(-1);
			}

			printf("\n");		
			secBufferMAC[thisEnv->id][0] = 0x00;				// set CTRL			just use frame type bit
			secBufferMAC[thisEnv->id][2] = (1<<4) | (thisEnv->id);		// SRC  = my addr 
			secBufferMAC[thisEnv->id][3] = thisEnv->addrInt;		// SRC  = my addr
			secBufferMAC[thisEnv->id][4] = dest[0];				// DEST = broadcast message
			secBufferMAC[thisEnv->id][5] = dest[1];				
			secBufferMAC[thisEnv->id][6] = 0x2C;				// set len = 1 + 4 + 33 + 2 + 4(type + ctr + DH + G.A. + MAC)

			// for EXT frames an additional octet (TPCI) follows
			secBufferMAC[thisEnv->id][7] = 0x00;				

			incGlobalCount(env);

			// assemble the payload
			secBufferMAC[thisEnv->id][8] = type;				// SEC HEADER	=~	ACPI
			secBufferMAC[thisEnv->id][9] = thisEnv->secGlobalCount[0];	// global Counter
			secBufferMAC[thisEnv->id][10] = thisEnv->secGlobalCount[1];	// ...
			secBufferMAC[thisEnv->id][11] = thisEnv->secGlobalCount[2];	// ...
			secBufferMAC[thisEnv->id][12] = thisEnv->secGlobalCount[3];	// global counter

			// append DH key
			for(i=0;i<DHPUBKSIZE;i++)
			{
//				printf("%02X ", dhPubKey[i]);
				secBufferMAC[thisEnv->id][i+13] = dhPubKey[i];
			}

			// append the wanted group adress				FIXME:		encrypt FIRST!!
			secBufferMAC[thisEnv->id][13+DHPUBKSIZE] = destGA[0];
			secBufferMAC[thisEnv->id][13+DHPUBKSIZE+1] = destGA[1];
			len = 13 + DHPUBKSIZE + 2;				// static stuff + DH key + wanted GA

			i = generateHMAC(secBufferMAC[thisEnv->id], len, &sigHMAC[thisEnv->id], &thisEnv->slen, thisEnv->skey);
			assert(i == 0);
			if(i != 0)
			{
				printf("SEC%d: FATAL, generateMAC() failed, exit\n", thisEnv->id);
				exit(-1);
			}
			// append MAC
			for(i = 0; i < MACSIZE; i++)
			{
				secBufferMAC[thisEnv->id][i+2+7+4+33+2] = sigHMAC[thisEnv->id][i];
			}
			// call write thread directly from here
			printf("SEC%d: writing discovery Request\n", thisEnv->id);
								// secHdr + TPCI + CTR + DH + G.A. + MAC
			if(type == discReq)
				secWRnew((char *)&secBufferMAC[thisEnv->id][7], (1+1+4+DHPUBKSIZE+2+4), discRes, env, NULL);
			else
				secWRnew((char *)&secBufferMAC[thisEnv->id][7], (1+1+4+DHPUBKSIZE+2+4), discRes, env, &dest[0]);
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

int secWRnew(char *buf, uint8_t len, uint8_t type, void *env, uint8_t *dest)
{
	threadEnvSec_t *thisEnv = (threadEnvSec_t *)env;
	eibaddr_t destEib = 0x0;

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
				printf("\tSEC%d-WR: REQUEST to FD @ %p\n", thisEnv->id, thisEnv->secFDWR);
			#endif

		break;
		case syncRes:
			destEib = dest[0]<<8 | dest[1];
			// this is a UNICAST message
			if ((EIBOpenT_Individual(thisEnv->secFDWR, destEib, FALSE)) == -1)
			{
				printf("SEC%d-WR: EIBOpenT_Individual() failed\n\n", thisEnv->id);
				exit(-1);
			}
			if (EIBSendAPDU(thisEnv->secFDWR, len, (const uint8_t *)buf) == -1)
			{
				printf("EIBOpenSendAPDU() failed\n\n");
       				exit(-1);
			}
			//#ifdef DEBUG
			//	printf("\tSEC%d-WR: RESPONSE to FD @ %p\n", thisEnv->id, thisEnv->secFDWR);
			//#endif

		break;
		case discReq:
			printf("SEC%d-WR: GOT %d bytes to write\n\n", thisEnv->id, len);
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
		break;
		case discRes:
			printf("SEC%d-WR: GOT %d bytes to write\n\n", thisEnv->id, len);
			destEib = dest[0]<<8 | dest[1];
			if ((EIBOpenT_Individual(thisEnv->secFDWR, destEib, 0)) == -1)
			{
				printf("SEC%d-WR: EIBOpenT_Broadcast() failed\n\n", thisEnv->id);
				exit(-1);
			}
			if (EIBSendAPDU(thisEnv->secFDWR, len, (const uint8_t *)buf) == -1)
			{
				printf("EIBOpenSendTPDU() failed\n\n");
       				exit(-1);
			}
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
						if(tmp.indivAdr)
						{
							thisEnv->secRDbuf[5] &= 0x0F;	// zero-out TTL, which gets changed by routers
						}
						else
						{
							thisEnv->secRDbuf[5] &= 0x8F;	// zero-out TTL, which gets changed by routers
						}
						//	check the MAC, use the first x bytes: header + payload (withouth MAC, without FCK)
						i = verifyHMAC(thisEnv->secRDbuf, (rc - MACSIZE - 1), &thisEnv->secRDbuf[rc-5], MACSIZE, thisEnv->skey);

						if(i != 0)
						{
							printf("\tSEC%d-RD: verifyHMAC() failed/attack?\n", thisEnv->id);
							for(i=0; i<rc;i++)
							{
								printf("%02x ", thisEnv->secRDbuf[i]);
							}
							printf(" / %d bytes total\n", rc);
						}
						else
						{
							// MAC is OK - process message
							// write src-address + payload to pipe - not interested in the rest of the header
							thisEnv->secRDbuf[4] = thisEnv->secRDbuf[1];
							thisEnv->secRDbuf[5] = thisEnv->secRDbuf[2];
							// total-bytes - (header - src) - MACSIZE - FCK
							printf("\tSEC%d-RD: writing %d + 2 bytes to pipe %d %d\n", thisEnv->id, rc-6-MACSIZE-1, thisEnv->secRDbuf[4], thisEnv->secRDbuf[5]);
							write(thisEnv->RD2MasterPipe[WRITEEND], &thisEnv->secRDbuf[4], rc - 4 - MACSIZE - 1);
						}
					}
					else if(tmp.type == extFrame)
					{
						thisEnv->secRDbuf[0] &= 0x80;	// zero-out repeat flag + priority	HATSCH
						if(tmp.indivAdr)
						{
							thisEnv->secRDbuf[1] &= 0x00;	// zero-out TTL, which gets changed by routers
						}
						else
						{
							thisEnv->secRDbuf[1] &= 0x10;	// zero-out TTL, which gets changed by routers
						}
						i = verifyHMAC(thisEnv->secRDbuf, (rc - MACSIZE - 1), &thisEnv->secRDbuf[rc-5], MACSIZE, thisEnv->skey);

						if(i != 0)
						{
							printf("\tSEC%d-RD: verifyHMAC() failed/attack?\n", thisEnv->id);
							for(i=0; i<rc;i++)
							{
								printf("%02x ", thisEnv->secRDbuf[i]);
							}
							printf(" / %d bytes total\n", rc);
						}
						else
						{
							// write src-address + payload to pipe - not interested in the rest of the header
							thisEnv->secRDbuf[6] = thisEnv->secRDbuf[2];
							thisEnv->secRDbuf[7] = thisEnv->secRDbuf[3];
							// MAC is OK, write, write data to pipe
							write(thisEnv->RD2MasterPipe[WRITEEND], &thisEnv->secRDbuf[6], rc - 6 - MACSIZE - 1);
						}


					}
					else
					{	
						printf("SEC%d-RD: got unknown frame type\n",thisEnv->id);
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
	uint8_t buffer[BUFSIZE], src[2], dest[2], i=0; 
	eibaddr_t srcEIB, destEIB;

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
				preparePacket(env, syncReq, NULL, NULL, NULL);
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
					read(thisEnv->RD2MasterPipe[READEND], &buffer[0], 3);
					if(buffer[2] == syncRes)
					{
						rc = read(thisEnv->RD2MasterPipe[READEND], &buffer[0], 8);
						if(rc == 8)
						{
							rc = checkFreshness(thisEnv, &buffer[4]);
							if(rc)
							{
								#ifdef DEBUG
									printf("SEC%d: FRESH sync response - counter = ", thisEnv->id);
									printf("%02x ", buffer[0]);
									printf("%02x ", buffer[1]);
									printf("%02x ", buffer[2]);
									printf("%02x\n", buffer[3]);
								#endif
								saveGlobalCount(thisEnv, &buffer[0]);
								thisEnv->state = STATE_READY;
							}
							else
							{
								thisEnv->retryCount++;
								thisEnv->state = STATE_SYNC_REQ;
								#ifdef DEBUG
									printf("SEC%d: OUTDATED sync response\n", thisEnv->id);
								#endif
							}
						}
						else
						{
							#ifdef DEBUG
								printf("SEC%d: expected 10 byte syncResp, got %d\n", thisEnv->id, rc);
							#endif
							thisEnv->retryCount++;
							thisEnv->state = STATE_SYNC_REQ;
						}
					}
					else
					{
						thisEnv->retryCount++;
						thisEnv->state = STATE_SYNC_REQ;
						// empty RD2MasterPipe buffer!!	FIXME: correct???
						read(thisEnv->RD2MasterPipe[READEND], &buffer[0], sizeof(buffer));
						printf("SEC%d: need syncResponse, got shit - FIXME\n", thisEnv->id);
					}
				}
			break;

			// looks like this node is alone - reset global counter
			// FIXME:	maybe choose random counter?
			case STATE_RESET_CTR:
				#ifdef DEBUG
					printf("SEC%d: RESET_CTR\n", thisEnv->id);
				#endif

				buffer[0] = 0x01;	// highest digit
				buffer[1] = 0x02;
				buffer[2] = 0x03;	
				buffer[3] = 0x04;	// lowest digit
				saveGlobalCount(thisEnv, &buffer[0]);
				thisEnv->state = STATE_READY;

			break;

			// node is ready to process datagrams
			case STATE_READY:
				#ifdef DEBUG
					printf("SEC%d: key master ready\n", thisEnv->id);
				#endif
				while(1)
				{
					/*
						using select() with timeout. every timeout executes a cleanup run to de-activate old discoveryrequest-entries
							FIXME:	every time new data arrives, the timeout is reset. this delays the cleanup run, or disables them
							totally if data arrives faster than the timeout occurs

					*/
					FD_ZERO(&thisEnv->set);
					FD_SET(thisEnv->RD2MasterPipe[READEND], &thisEnv->set);

					//	instead of setting this to constant values, set to diff from last run to keep cleanup runs at constant basis
					syncTimeout.tv_sec = 2;
					syncTimeout.tv_usec = 0;	

					selectRC = select(FD_SETSIZE, &thisEnv->set, NULL, NULL, &syncTimeout);
					// 	select() updates tv_sec, tv_usec, this can be used to re-calculcate a new timeout if necessary		
					//printf("SEC%d: remainting time for sleep: %ld,%ld\n", thisEnv->id, syncTimeout.tv_sec, syncTimeout.tv_usec);

					// whenever timeout occurs here - cleanup recent active discovery-request stuff
					if(selectRC == 0)
					{
	/*
						#ifdef DEBUG
							printf("SEC%d: cleanup run now...\n", thisEnv->id);
						#endif
	*/
						;;
					}
					// error occured
					else if(selectRC < 0)
					{
						// error
						printf("%s - ", strerror(errno));
						#ifdef DEBUG
							printf("SEC%d: cleanup / select() error\n", thisEnv->id);
						#endif
					}
					else
					{
						// read src + type
						read(thisEnv->RD2MasterPipe[READEND], &buffer[0], 3);	// FIXME - must be non-blocking!!
						switch(buffer[2])
						{
							case syncReq:
								// save src address from last frame
								src[0] = buffer[0];
								src[1] = buffer[1];
	
								rc = read(thisEnv->RD2MasterPipe[READEND], &buffer[0], 4);	// FIXME - non-blocking
								if(rc == 4)
								{
									rc = checkFreshness(thisEnv, &buffer[0]);
									if(rc)
									{
										printf("SEC%d: got fresh syncReq, reply to %d.%d.%d\n", thisEnv->id, (src[0]>>4), src[0]&0x0F, src[1]);
										preparePacket(env, syncRes, &src[0], NULL, NULL);
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
	
							break;
	
							case discReq:				
								// save src address from last frame
								src[0] = buffer[0];
								src[1] = buffer[1];

								rc = read(thisEnv->RD2MasterPipe[READEND], &buffer[0], 4+33+2);	// FIXME - non-blocking
								printf("SEC%d: ", thisEnv->id);
								for(i=0; i < 39;i++)
									printf("%02X ", buffer[i]);

								printf("\n");
								#ifdef DEBUG
									printf("SEC%d: discovery REQUEST - counter = ", thisEnv->id);
									printf("%02x ", buffer[0]);
									printf("%02x ", buffer[1]);
									printf("%02x ", buffer[2]);
									printf("%02x\n", buffer[3]);
								#endif
								if(saveGlobalCount(env, &buffer[0]))
								{
		//FIXME: decrypt G.A.

									// is this GW responsible for the received G.A.?
									if(checkGA(env, &buffer[4+33]))
									{
										printf("we are responsible\n");
										for(i=0;i<10;i++)			// FIXME: this is dirty - only memory for 10 knx sending devices!!
										{
											if(thisEnv->indCounters[i].src == srcEIB)
											{
												printf(" / found src at index %02d, counter = %02d\n", i, thisEnv->indCounters[i].indCount);
												// do NOT update counter now - only AFTER the actual knx packet gets delivered
												//thisEnv->indCounters[i].indCount;
												break;
											}
											if(thisEnv->indCounters[i].src == 0x00)
											{
												printf(" / NOT found, adding src at index %d\n", i);
												thisEnv->indCounters[i].src = srcEIB;
												thisEnv->indCounters[i].indCount = 0x01;
												thisEnv->indCounters[i].pkey = EVP_PKEY_new();
												break;
											}
										}		
										genECpubKey(thisEnv->indCounters[i].pkey, thisEnv->indCounters[i].myPubKey);
										dest[0] = buffer[37];
										dest[1] = buffer[38];
										preparePacket(thisEnv, discRes, &src[0], &dest[0], thisEnv->indCounters[i].myPubKey);
		// FIXME: at this point, we can already calculate the shared secret on this side

									}
									else
									{
										printf("NOT responsible\n");
									}
								}
							break;
	
							case clrData:
								// suck in the whole cleartext knx frame
								rc = read(thisEnv->RD2MasterPipe[READEND], &buffer[0], BUFSIZE);	// FIXME - non-blocking
								srcEIB = (buffer[1]<<8) | buffer[2];
								destEIB = (buffer[3]<<8) | buffer[4];		// this is the wanted group address we want to resolve
	/*
								#ifdef DEBUG
									printf("DIS-%d: got %03d byte %04d->%04d: ", thisEnv->id, rc, srcEIB, destEIB);
									for(i=0; i < rc; i++)
									{
										printf("%02X ", buffer[i]);
									}
									//printf("\n");
								#endif
	*/
	
								for(i=0;i<10;i++)			// FIXME: this is dirty - only memory for 10 knx sending devices!!
								{
									if(thisEnv->indCounters[i].src == srcEIB)
									{
										printf(" / found src at index %02d, counter = %02d\n", i, thisEnv->indCounters[i].indCount);
										thisEnv->indCounters[i].indCount++;							// FIXME: handle overflow
										break;
									}
									if(thisEnv->indCounters[i].src == 0x00)
									{
										printf(" / NOT found, adding src at index %d\n", i);
										thisEnv->indCounters[i].src = srcEIB;
										thisEnv->indCounters[i].indCount = 0x01;
										thisEnv->indCounters[i].pkey = EVP_PKEY_new();
										break;
									}
								}		
								thisEnv->indCounters[i].dest = destEIB;
								thisEnv->indCounters[i].active = TRUE;;
			
								genECpubKey(thisEnv->indCounters[i].pkey, thisEnv->indCounters[i].myPubKey);
								dest[0] = 0x00;
								dest[1] = 0x00;
								preparePacket(thisEnv, discReq, &dest[0], &buffer[3], thisEnv->indCounters[i].myPubKey);
								break;

							default: 	
								printf("SEC%d: got unknown input\n", thisEnv->id);
						}
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
void initSec(void *threadEnv)
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
}
