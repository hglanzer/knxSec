#include<stdio.h>
#include<signal.h>
#include<stdlib.h>
#include<eibclient.h>
#include "timer.h"

#define AreaAddress(adr) ((uint8_t)(adr >> 4 ))
#define LineAddress(adr) ((uint8_t)(adr & 0x0F))

#define isGroupaddress(adr) ((uint8_t)(adr & (1<<8)))

void sigalrm_handler(int sig)
{
	printf("ALARM\n");
	alarm(1);
}


struct packetStruct
{
	uint8_t ctrl;
	uint8_t srcAreaLine;
	uint8_t srcDev;
	uint8_t dst[2];
	uint8_t atNcpiLength;
	uint8_t payload[25];
};

int main(void)
{
	int len = 0;
	struct packetStruct *packet;
	EIBConnection *master;
/*
	signal(SIGALRM, &sigalrm_handler);
	alarm(1);
*/
	packet = malloc(sizeof(struct packetStruct));
	master = EIBSocketURL("local:/tmp/eib");

	if (master != NULL)
		printf("OK - socket opened\n");
	else
	{
		printf("NOTOK\n");
		return -1;
	}
	
	if(EIBOpenBusmonitor(master) == 0)
	{
		printf("OK - busmonitor started\n");
	}
	else
	{
		printf("cannot open T_Connection\n");
		return -1;
	}

	while(1)
	{
		len = EIBGetBusmonitorPacket (master, sizeof(*packet), (uint8_t *)packet);
		if (len == -1)
		{
			printf("GET failed\n");
			return -1;
		}

		printf("%x\n", packet->atNcpiLength & 0x80);
		printf("Len = %d / CTRL: %x / %x.%x.%x", len, packet->ctrl, \
			AreaAddress(packet->srcAreaLine), LineAddress(packet->srcAreaLine), packet->srcDev);

		if(((packet->atNcpiLength) & 0x80) == 1)
		//if(isGroupaddress(packet->atNcpiLength) == 1)
		{
			printf(" -> %x / %x", packet->dst[0], packet->dst[1]);
		}
		else
		{
			printf(" -> %x . %x",  packet->dst[0], packet->dst[1]);
		}
		printf(" atNcpiLength = %x\n", packet->atNcpiLength);

		fflush (stdout);
	}

	EIBClose(master);

	return 0;
}
