#include "common.h"

#define TRUE	1
#define FALSE	0

// bit definitions
#define frameType	7
#define repeatedFrame	5

// field definitions
#define ctrlField	0
#define srcField1	1
#define srcField2	2
#define destField1	3
#define destField2	4
//	MACROS
#define srcAreaAddress(frame)		(frame[srcField1] >> 4)
#define srcLineAddress(frame)		((frame[srcField1] & 0x0F))
#define srcDeviceAddress(frame)		(frame[srcField2])

#define destAreaAddress(frame)		((frame[destField1] >> 4 ))
#define destLineAddress(frame)		((frame[destField1] & 0x0F))
#define destDeviceAddress(frame)	(frame[destField2])

#define isRepeated(frame) ((frame[ctrlField] >> repeatedFrame) ^ 0x01)

#define isStdFrame(frame) ((frame[ctrlField] >> frameType) & 0x01)
#define isExtFrame(frame) ((frame[ctrlField] >> frameType) ^ 0x01)

// Standard Frame specific macros
// addr type on different locations for std / ext frames
#define isGroupAddrStd(frame)		((frame[5] >> 7) & 0x01)
#define isIndivAddrStd(frame)		((frame[5] >> 7) ^ 0x01)
#define lengthStd(frame)		(frame[5] & 0x0F)
#define hopcountStd(frame)		((frame[5] >> 4)& 0x07)		// = NCPI
#define tpciStd(frame)			(frame[6] >> 2)			// see 03_03_4_ Transport Layer...pdf

int decodeFrame(uchar *frame)
{
	if(isStdFrame(frame))
	{
		printf("STANDARD FRAME: payload=%d / hopcount=%d / TCPI=%d / ", lengthStd(frame), hopcountStd(frame), tpciStd(frame));
		printf("%d.%d.%d -> ", srcAreaAddress(frame), srcLineAddress(frame),srcDeviceAddress(frame));	
		if(isIndivAddrStd(frame))	// std frame for indiv. address
		{
			printf("%d.%d.%d -> ", destAreaAddress(frame), destLineAddress(frame), destDeviceAddress(frame));	
		}
		else			// std frame for group address
		{
			printf("%d/%d/%d", destAreaAddress(frame), destLineAddress(frame), destDeviceAddress(frame));	
		}

	}
	else	// extended frame received
	{
		printf("EXT frame ");
	/*
		if(isIndivAddrStd)	// extended frame for indiv. address
		{

		}
		else			// extended frame for group address
		{

		}
	*/
	}
	printf("\n");
	return 0;	
}

int main (int ac, char *ag[])
{
	uchar buf[255];
	buf[0] = 0xFA;
	buf[1] = 0xBC;
	buf[2] = 0x3;
	buf[3] = 0x4;
	int rc=0, i=0;
	eibaddr_t myAddr = 22;
	eibaddr_t dest = 11;
	EIBConnection *con;

	if (ac != 2)
	{
		printf("usage: %s <EIB-URL>\n\n", ag[0]);
        	return -1;
	}
	con = EIBSocketURL (ag[1]);
	if (!con)
	{
		printf("EIBSocketURL() failed\n\n");
        	return -1;
	}
	printf("URL opened\n\n");

	if (EIBOpenBusmonitor(con) == -1)
	//if (EIBOpenVBusmonitor(con) == -1)
	{
		printf("EIBOpenBusmonitor() failed\n\n");
	       	return -1;
	}

	printf("entering loop\n");
	while(1)
	{
		printf("waiting...");
		rc = EIBGetBusmonitorPacket(con, sizeof(buf), buf);
		if(rc == -1)
		{
			printf("EIBGetBusmonitorPacket() failed\n\n");
        		return -1;
		}
		else
		{
			for(i=0; i < rc; i++)
				printf("%02X ", buf[i]);


			decodeFrame(buf);
		}
	}

	EIBClose (con);
	printf("con closed\n\n");
	return 0;
}
