#include "globals.h"

/*
	harald glanzer, 0727156
	harald.glanzer@gmail.com

	needs 3 running eib daemones:
		eibd --listen-local=/tmp/eib.clr  -t31 -e 1.0.<addr>  tpuarts:/dev/tty<DEV-cleartext>
		eibd --listen-local=/tmp/eib.sec1 -t31 -e 1.1.<addr> tpuarts:/dev/tty<DEV-secured-1>
		eibd --listen-local=/tmp/eib.sec2 -t31 -e 1.2.<addr>  tpuarts:/dev/tty<DEv-secured-2>
*/

pthread_mutex_t mainMutex;

pthread_mutex_t SecMutexWr[SECLINES];
pthread_cond_t  SecCondWr[SECLINES];

byte secBufferMAC[SECLINES][BUFSIZE];
byte secBufferWr[SECLINES][BUFSIZE];
byte secRDbuf[SECLINES][BUFSIZE];
byte secBufferTime[SECLINES][BUFSIZE];

EVP_PKEY *skey[SECLINES];
EVP_PKEY *vkey[SECLINES];
size_t slen[SECLINES];
size_t vlen[SECLINES];
byte *sigHMAC[SECLINES];
struct msgbuf_t MSGBUF_SEC2WR[SECLINES];
uint32_t globalCount[SECLINES];

time_t now[SECLINES];

threadEnvSec_t threadEnvSec[SECLINES]; 
threadEnvClr_t threadEnvClr; 

static struct option long_options[] =
{
	/* These options set a flag. */
	//{"verbose", no_argument,       &verbose_flag, 1},
	//{"brief",   no_argument,       &verbose_flag, 0},
	/* These options don’t set a flag.
	   We distinguish them by their indices. */
	{"help",	no_argument,		NULL, 'h'},
	{"clrSocket",	required_argument,	NULL, 'c'},
	{"sec1Socket",	required_argument,	NULL, '1'},
	{"sec2Socket",	required_argument,	NULL, '2'},
	{"addr",required_argument,	NULL, '3'},
	{0, 0, 0, 0}
};

void Usage(char **argv)
{
	printf("\n\n\nUsage: %s --clrSocket local:<socket> --sec1Socket local:<socket> --sec2Socket local:<socket> --addr <device addr>\n", argv[0]);
	printf("\tDevice address must be in range [1,15]:\n");
	printf("\t\tSEC0 Line: 1.0.<device addr>\n");
	printf("\t\tSEC1 Line: 1.1.<device addr>\n");
	printf("\t\tCLR  Line: 1.2.<device addr>\n\n");
	exit(EXIT_FAILURE);
}

/*
	master prozess: creates 3 threads:
		- 2 secLine threads
		- 1 clrThreadLine threads

*/
int main(int argc, char **argv)
{
	int c, i=0, option_index = 0, tmp = 0, dig=0;
	
	/*
		thread - variables
	*/
	
	int (*clrMasterStart)(void *);
	clrMasterStart = &initClr;

	int (*secMasterStart)(void *);
	secMasterStart = &initSec;

	void (*secRDThreadfPtr)(void *);
	secRDThreadfPtr = &secRD;

	void *clrThreadRetval, *sec1ThreadRetval, *sec2ThreadRetval;
	pthread_t sec1MasterThread, sec2MasterThread, clrMasterThread;
	pthread_t sec1RDThread, sec2RDThread;

	pthread_mutexattr_t mutexAttr;
	pthread_mutexattr_init(&mutexAttr);
	if((pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE_NP)) != 0)
		printf("mutex set attr failed\n");
	
	while((c = getopt_long(argc, argv, "hc:1:2:3:4:", long_options, &option_index)) != EOF)
	//while((c = getopt(argc, argv, "hs:")) != EOF)
	{
		switch (c)
		{
			case 'h':
				Usage(argv);
			break;

			case '?':
				Usage(argv);
			break;

			// options	END
			// arguments 	START
			case '1':
				printf("\tARG: %20s for SEC1 SOCKET\n", &optarg[0]);
				threadEnvSec[0].socketPath = &optarg[0];	
				threadEnvSec[0].id = SEC1;
			break;
			case '2':
				printf("\tARG: %20s for SEC2 SOCKET\n", &optarg[0]);
				threadEnvSec[1].socketPath = &optarg[0];	
				threadEnvSec[1].id = SEC2;
			break;
			case '3':
				printf("\tARG: %d for CLR/SEC1/SEC2 Device Addr\n", optarg[0]);
				// addr scheme 1.<SEC#>.<addr>
				for(i=(strlen(optarg)-1);i >= 0;i=i-1)
				{
					dig = (int)pow(10,(strlen(optarg)-(i+1)));
					tmp = tmp + (optarg[i]-0x30)*dig;
				}
			
				if((tmp < 16 ) && (tmp > 0))	
				{
					threadEnvClr.addrInt  = optarg[0];			//1.0.<addr> 
					threadEnvSec[0].addrInt = optarg[0];			//1.1.<addr>
					threadEnvSec[1].addrInt = optarg[0];			//1.2.<addr>
				}
				else
				{
					Usage(argv);
				}
			break;
			case 'c':
				printf("\tARG: %20s for CLR  SOCKET\n", &optarg[0]);
				threadEnvClr.socketPath = &optarg[0];	
			break;
			default:
				assert(0);
		}
	}

	for(i=0; i < SECLINES; i++)
	{
		// create condition variables for syncronization clr(recv) -> sec(send)
		if(pthread_cond_init(&SecCondWr[i], NULL) != 0)
		{
			printf("condition variable %d init failed, exit", i);
			return -1;
		}

		// every condition variable needs a mutex
		if(pthread_mutex_init(&SecMutexWr[i], NULL) != 0)
		{
			printf("clr mutex init failed, exit");
			return -1;
		}
	}
	if(pthread_mutex_init(&mainMutex, NULL) != 0)
	{
		printf("mainMutex init failed, exit");
		return -1;
	}


/*
	// create cleartext-knx master thread
	if((pthread_create(&clrMasterThread, NULL, (void *)clrMasterStart, &threadEnvClr)) != 0)
	{
		printf("clrThread thread init failed, exit\n");
		return -1;
	}
*/
	// create secure-knx master thread 1	
	if((pthread_create(&sec1MasterThread, NULL, (void *)secMasterStart, &threadEnvSec[0])) != 0)
	{
		printf("sec1Thread thread init failed, exit\n");
		return -1;
	}
	//sleep(1);
	// create secure-knx master thread 2
	if((pthread_create(&sec2MasterThread, NULL, (void *)secMasterStart, &threadEnvSec[1])) != 0)
	{
		printf("sec2Thread thread init failed, exit\n");
		return -1;
	}
	// create READ thread

	if((pthread_create(&sec1RDThread, NULL, (void *)secRDThreadfPtr, &threadEnvSec[0])) != 0)
	{
		printf("sec RECEIVE Thread init failed, exit\n");
		exit(-1);
	}
	if((pthread_create(&sec2RDThread, NULL, (void *)secRDThreadfPtr, &threadEnvSec[1])) != 0)
	{
		printf("sec RECEIVE Thread init failed, exit\n");
		exit(-1);
	}
	#ifdef DEBUG
		printf("\n\nMaster   Thread %u, waiting for kids\n", (unsigned)pthread_self());
		printf("sec1Mst  Thread: %u\n", (unsigned)sec1MasterThread);
		printf("sec2Mst  Thread: %u\n", (unsigned)sec2MasterThread);
		printf("clearMst Thread: %u\n\n\n", (unsigned)clrMasterThread);
	#endif
//	pthread_join(clrMasterThread, &clrThreadRetval);
	pthread_join(sec1MasterThread, &sec1ThreadRetval);
	pthread_join(sec2MasterThread, &sec2ThreadRetval);
	pthread_join(sec1RDThread, &sec1ThreadRetval);
	pthread_join(sec2RDThread, &sec2ThreadRetval);

	#ifdef DEBUG
		printf("all threads gone, exit\n");
	#endif

	return 0;
}
