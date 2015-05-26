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
	int len, i=0;
	eibaddr_t myAddr = 22;
	eibaddr_t dest = 11;
	EIBConnection *con;

	if (ac != 3)
	{
		printf("usage: %s <EIB-URL> <g(group)/i(individual)/b(broadcast)>\n\n", ag[0]);
        	return -1;
	}
	con = EIBSocketURL (ag[1]);
	if (!con)
	{
		printf("EIBSocketURL() failed\n\n");
        	return -1;
	}

	printf("URL opened\n\n");

	if(ag[2][0] == 'b')
	{

		if (EIBOpenVBusmonitor(con) == -1)
		{
			printf("EIBOpenBusmonitor() failed\n\n");
	        	return -1;
		}

		printf("busmonitor connection opened\n\n");

		if (EIBGetBusmonitorPacket(con, 2, buf) == -1)
		{
			printf("EIBGetBusmonitorpacket() failed\n\n");
        		return -1;
		}
		printf("got broadcast: %x%x", buf[0], buf[1]);
	}
	if(ag[2][0] == 'i')
	{

		if (EIBOpenT_TPDU(con, myAddr) == -1)
		{
			printf("EIBpenT_TPDU() failed\n\n");
	        	return -1;
		}

		printf("connection opened\n\n");

		if (EIBSendTPDU(con, dest, 2, buf) == -1)
		{
			printf("EIBpenSendTPDU() failed\n\n");
        		return -1;
		}
	}
	if(ag[2][0] == 'g')
	{
		if (EIBOpenT_Group(con, dest, TRUE) == -1)
		{
			printf("EIBpenT_Group) failed\n\n");
	        	return -1;
		}
		printf("connection opened\n\n");
		if (EIBSendGroup(con, dest, 2, buf) == -1)
		{
			printf("EIBpenSendGroup() failed\n\n");
        		return -1;
		}
	}

	EIBClose (con);
	return 0;
}
