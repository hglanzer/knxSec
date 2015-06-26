#include "globals.h"

int decodeFrame(unsigned char *frame, knxPacket *packet)
{
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
/*
		int i=0;
		for(i=0; i<lengthStd(frame); i++)
		{
			printf("%02x ", frame[STDpayloadField + i]);
		}
*/
		printf("\n");
	}
	else if(isExtFrame(frame))	// extended frame received
	{
		packet->type = extFrame;
		packet->srcDev = frame[3];
		
		printf("%d.%d.%d -> ", srcAreaAddressExt(frame), srcLineAddressExt(frame),srcDeviceAddressExt(frame));	
		if(isIndivAddrExt(frame))	// STD frame for indiv. address
		{
			packet->indivAdr = 1;
			printf("%d.%d.%d", destAreaAddressExt(frame), destLineAddressExt(frame), destDeviceAddressExt(frame));	
		}
		else			// Ext group / broadcast
		{
			packet->indivAdr = 0;
			printf("%d/%d/%d", destAreaAddressExt(frame), destLineAddressExt(frame), destDeviceAddressExt(frame));	
		}
		printf(" EXT: dataLen=%d / TTL=%d / ", lengthExt(frame), hopcountExt(frame));
/*
		int i=0;
		#ifdef DEBUG
		for(i=0; i<lengthExt(frame); i++)
		{
			printf("%02x ", frame[EXTpayloadField + i]);
		}
		#endif
*/
		printf("\n");
	}
	else
	{
		printf("got unknown type\n");
	}
	return 0;	
}
