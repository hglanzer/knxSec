#include "common.h"

#define TRUE	1
#define FALSE	0

#define AreaAddress(adr) ((uint8_t)(adr >> 4 ))
#define LineAddress(adr) ((uint8_t)(adr & 0x0F))

main (int ac, char *ag[])
{
	uchar buf[255];
	int size, i=0;
	eibaddr_t myAddr = 22;
	eibaddr_t dest = 11;
	EIBConnection *con;

	size = 10;
	for(i=0; i<size;i++)
	{
		buf[i] = i+1;
	}
	buf[i] = '\0';

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
	sleep(2);

	if(ag[2][0] == 'b')
	{

		if (EIBOpenT_Broadcast(con, 0) == -1)
		{
			printf("EIBpenT_Broadcast() failed\n\n");
	        	return -1;
		}

		printf("broadcast connection opened\n\n");
		sleep(2);

		dest = 0x00;
		//if (EIBSendTPDU(con, dest, size, buf) == -1)
		if (EIBSendAPDU(con, size, buf) == -1)
		{
			printf("EIBpenSendTPDU() failed\n\n");
        		return -1;
		}
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
