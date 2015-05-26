#include "common.h"

#define TRUE	1
#define FALSE	0

#define AreaAddress(adr) ((uint8_t)(adr >> 4 ))
#define LineAddress(adr) ((uint8_t)(adr & 0x0F))

main (int ac, char *ag[])
{
	uchar buf[255];
	buf[0] = 0xFA;
	buf[1] = 0xBC;
	buf[2] = 0x3;
	buf[3] = 0x4;
	int rc=0;
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

	if (EIBOpenVBusmonitor(con) == -1)
	{
		printf("EIBOpenBusmonitor() failed\n\n");
	       	return -1;
	}

	while(1)
	{
		EIBGetBusmonitorPacket(con, sizeof(buf), buf);
		if(rc == -1)
		{
			printf("EIBGetBusmonitorPacket() failed\n\n");
        		return -1;
		}
		else
		{
			printf("new package from monitor, %d byte\n", rc);
		}
	}

	EIBClose (con);
	printf("con closed\n\n");
	return 0;
}
