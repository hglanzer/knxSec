#include "common.h"

#define TRUE	1
#define FALSE	0

#define AreaAddress(adr) ((uint8_t)(adr >> 4 ))
#define LineAddress(adr) ((uint8_t)(adr & 0x0F))
#define EIBADDR(area,line,device) (((area << 4 | line) << 8) | device)

main (int ac, char *ag[])
{
	uchar buf[255];
	int size, i=0;
	eibaddr_t myAddr = 22;
	eibaddr_t dest = 0x0;
	EIBConnection *con;

	/*
		IF EIBSendAPDU is used it seems like the very first byte of the APDU is the APCI
	*/

	size = 5;
	buf[0] = 0x40;		// set APCI, first 2 bit
	buf[1] = 0x40;		// set APCI, first 2 bit
	for(i=2; i<size;i++)
	{
	//	buf[i] = i;
		buf[i] = 0x04;
	}
	//buf[i] = '\0';

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
	sleep(1);

	if(ag[2][0] == 'b')
	{
				// **************** use 1 to see traffic on remote side
		if (EIBOpenT_Broadcast(con, 1) == -1)
		{
			printf("EIBOpenT_Broadcast() failed\n\n");
	        	return -1;
		}

		printf("broadcast connection opened\n\n");
		sleep(1);

		dest = 0x00;
		//if (EIBSendTPDU(con, dest, size, buf) == -1)
		if (EIBSendAPDU(con, size, buf) == -1)
		{
			printf("EIBOpenSendTPDU() failed\n\n");
        		return -1;
		}
	}
	if(ag[2][0] == 'i')
	{
			
		dest = EIBADDR(2,4,120);
		if (EIBOpenT_Individual(con, dest, FALSE) == -1)
		//if (EIBOpenT_TPDU(con, dest) == -1)
		{
			printf("EIBpenT_TPDU() failed\n\n");
	        	return -1;
		}

		printf("connection opened\n\n");

		if (EIBSendAPDU(con, size, buf) == -1)
		//if (EIBSendTPDU(con, dest, size, buf) == -1)
		{
			printf("EIBpenSendTPDU() failed\n\n");
        		return -1;
		}
	}
	if(ag[2][0] == 'g')
	{
		dest = 0x1201;
		//if (EIBOpenT_Group(con, dest, FALSE) == -1)
		//if (EIBOpenT_Group(con, dest, TRUE) == -1)
		if (EIBOpen_GroupSocket(con, TRUE) == -1)
		{
			printf("EIBOpenT_Group() failed\n\n");
	        	return -1;
		}
		printf("connection opened\n\n");
		if (EIBSendGroup(con, dest, size, buf) == -1)
		{
			printf("EIBpenSendGroup() failed\n\n");
        		return -1;
		}
	}

	EIBClose (con);
	return 0;
}
