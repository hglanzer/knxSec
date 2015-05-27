#include "globals.h"

/*
	harald glanzer, 0727156
	harald.glanzer@gmail.com

	needs 3 running eib daemones:
		eibd --listen-local=/tmp/eib.clr  -t31 -e 1.1.x  tpuarts:/dev/tty<DEV-cleartext>
		eibd --listen-local=/tmp/eib.sec1 -t31 -e 1.1.y  tpuarts:/dev/tty<DEV-secured-1>
		eibd --listen-local=/tmp/eib.sec2 -t31 -e 1.1.z  tpuarts:/dev/tty<DEv-secured-2>
*/

uint8_t	clr2SecBUF[CLSBUFSIZE];
uint8_t	sec2ClrBUF[SECBUFSIZE];

pthread_mutex_t SecMutexWr[SECLINES];
pthread_cond_t  SecCondWr[SECLINES];

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
	{0, 0, 0, 0}
};

void Usage(char **argv)
{
	printf("Usage: %s --clrSocket local:<socket> --sec1Socket local:<socket> --sec2Socket local:<socket>\n", argv[0]);
	exit(EXIT_FAILURE);
}

void debug(char *str, pthread_t id)
{
	printf(" %u: %s\n", (unsigned)id, str);
}

/*
	master prozess: creates 3 threads:
		- 2 secLine threads
		- 1 clrThreadLine threads

*/
int main(int argc, char **argv)
{
	int c, i=0, option_index = 0;
	
	/*
		thread - variables
	*/
	struct threadEnvClr_t threadEnvClr; 
	struct threadEnvSec_t threadEnvSec1; 
	struct threadEnvSec_t threadEnvSec2; 
	
	int (*clrMasterStart)(void);
	clrMasterStart = &initClr;

	int (*secMasterStart)(void *);
	secMasterStart = &initSec;

	void *clrThreadRetval, *sec1ThreadRetval, *sec2ThreadRetval;
	pthread_t sec1MasterThread, sec2MasterThread, clrMasterThread;

	pthread_mutexattr_t mutexAttr;
	pthread_mutexattr_init(&mutexAttr);
	if((pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE_NP)) != 0)
		printf("mutex set attr failed\n");
	
	while((c = getopt_long(argc, argv, "hc:1:2:", long_options, &option_index)) != EOF)
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
				printf("\tARG: %s for SEC1 SOCKET\n", &optarg[0]);
				threadEnvSec1.socket = &optarg[0];	
				threadEnvSec1.id = SEC1;
			break;
			case '2':
				printf("\tARG: %s for SEC2  SOCKET\n", &optarg[0]);
				threadEnvSec2.socket = &optarg[0];	
				threadEnvSec2.id = SEC2;
			break;
			case 'c':
				printf("\tARG: %s for CLS SOCKET\n", &optarg[0]);
				threadEnvClr.socket = &optarg[0];	
			break;
			default:
				assert(0);
		}
	}

	for(i=0; i < 2; i++)
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


	// create cleartext-knx master thread
	if((pthread_create(&clrMasterThread, NULL, (void *)clrMasterStart, &threadEnvClr)) != 0)
	{
		printf("clrThread thread init failed, exit\n");
		return -1;
	}

	// create secure-knx master thread 1	
	if((pthread_create(&sec1MasterThread, NULL, (void *)secMasterStart, &threadEnvSec1)) != 0)
	{
		printf("sec1Thread thread init failed, exit\n");
		return -1;
	}

	// create secure-knx master thread 2
	if((pthread_create(&sec2MasterThread, NULL, (void *)secMasterStart, &threadEnvSec2)) != 0)
	{
		printf("sec2Thread thread init failed, exit\n");
		return -1;
	}

	#ifdef DEBUG
		printf("\n\nMaster   Thread %u, waiting for kids\n", (unsigned)pthread_self());
		printf("sec1Mst  Thread: %u\n", (unsigned)sec1MasterThread);
		printf("sec2Mst  Thread: %u\n", (unsigned)sec2MasterThread);
		printf("clearMst Thread: %u\n\n\n", (unsigned)clrMasterThread);
	#endif
	pthread_join(clrMasterThread, &clrThreadRetval);
	pthread_join(sec1MasterThread, &sec1ThreadRetval);
	pthread_join(sec2MasterThread, &sec2ThreadRetval);

	#ifdef DEBUG
		printf("all threads gone, exit\n");
	#endif

	return 0;
}
