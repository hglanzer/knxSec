#include "globals.h"

int decodeFrame(unsigned char *frame, knxPacket *packet)
{
	int i=0;
	if(isStdFrame(frame))
	{
		printf("%d.%d.%d -> ", srcAreaAddress(frame), srcLineAddress(frame),srcDeviceAddress(frame));	
		packet->type = stdFrame;
		packet->srcDev = srcDeviceAddress(frame);
		packet->srcLine = srcLineAddress(frame);
		packet->srcArea = srcAreaAddress(frame);
		if(isIndivAddrStd(frame))	// std frame for indiv. address
		{
			packet->indivAdr = 1;
			printf("%d.%d.%d", destAreaAddress(frame), destLineAddress(frame), destDeviceAddress(frame));	
		}
		else			// std frame for group address
		{
			packet->indivAdr = 0;
			printf("%d/%d/%d", destAreaAddress(frame), destLineAddress(frame), destDeviceAddress(frame));	
		}
		printf(" STD: dataLen=%d / TTL=%d / TCPI=%d / ", lengthStd(frame), hopcountStd(frame), tpciStd(frame));
		for(i=0; i<lengthStd(frame); i++)
		{
			printf("%02x ", frame[STDpayloadField + i]);
		}
		printf("\n");
	}
	else if(isExtFrame(frame))	// extended frame received
	{
		printf("EXT frame ");
		packet->type = extFrame;
		packet->srcDev = frame[3];
	/*
		if(isIndivAddrStd)	// extended frame for indiv. address
		{

		}
		else			// extended frame for group address
		{

		}
	*/
	}
	else
	{
		printf("got unknown type\n");
	}
	return 0;	
}
