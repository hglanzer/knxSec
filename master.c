#include "globals.h"

/*
	harald glanzer, 0727156
	harald.glanzer@gmail.com
*/

uint8_t	cls2SecBUF[CLSBUFSIZE];
uint8_t	sec2ClsBUF[SECBUFSIZE];

pthread_mutex_t cls2SecMutexWr[2];
pthread_cond_t  cls2SecCondWr[2];

int newData4Sec = 0;
int newData4Cls = 0;
int count = 0;

static struct option long_options[] =
{
	/* These options set a flag. */
	//{"verbose", no_argument,       &verbose_flag, 1},
	//{"brief",   no_argument,       &verbose_flag, 0},
	/* These options don’t set a flag.
	   We distinguish them by their indices. */
	{"help",	no_argument,		NULL, 'h'},
	{"clsSocket",	required_argument,	NULL, 'c'},
	{"sec1Socket",	required_argument,	NULL, '1'},
	{"sec2Socket",	required_argument,	NULL, '2'},
	{0, 0, 0, 0}
};

void Usage(char **argv)
{
	printf("Usage: %s --clsSocket local:<socket> --sec1Socket local:<socket> --sec2Socket local:<socket>\n", argv[0]);
	exit(EXIT_FAILURE);
}

void debug(char *str, pthread_t id)
{
	printf(" %u: %s\n", (unsigned)id, str);
}

/*
	master prozess: creates 3 threads:
		- 2 secLine threads
		- 1 clsThreadLine threads

*/
int main(int argc, char **argv)
{
	int c, i=0, option_index = 0;
	
	/*
		thread - variables
	*/
	struct threadEnvCls_t threadEnvCls; 
	struct threadEnvSec_t threadEnvSec1; 
	struct threadEnvSec_t threadEnvSec2; 
	
	int (*clsThreadStart)(void);
	clsThreadStart = &initCls;

	int (*secStart)(void *);
	secStart = &initSec;

	void *clsThreadRetval, *sec1ThreadRetval, *sec2ThreadRetval;
	pthread_t sec1MasterThread, sec2MasterThread, clsMasterThread;

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
				threadEnvCls.socket = &optarg[0];	
			break;
			default:
				assert(0);
		}
	}
	#ifdef DEBUG
		printf("creating threads\n");
	#endif

	for(i=0; i < 2; i++)
	{
		// create condition variables for syncronization cls(recv) -> sec(send)
		if(pthread_cond_init(&cls2SecCondWr[i], NULL) != 0)
		{
			printf("condition variable %d init failed, exit", i);
			return -1;
		}

		// every condition variable needs a mutex
		if(pthread_mutex_init(&cls2SecMutexWr[i], NULL) != 0)
		{
			printf("cls mutex init failed, exit");
			return -1;
		}
	}


	// create cleartext-knx master thread
	if((pthread_create(&clsMasterThread, NULL, (void *)clsThreadStart, &threadEnvCls)) != 0)
	{
		printf("clsThread thread init failed, exit\n");
		return -1;
	}

	// create secure-knx master thread 1	
	if((pthread_create(&sec1MasterThread, NULL, (void *)secStart, &threadEnvSec1)) != 0)
	{
		printf("sec1Thread thread init failed, exit\n");
		return -1;
	}

	// create secure-knx master thread 2
	if((pthread_create(&sec2MasterThread, NULL, (void *)secStart, &threadEnvSec2)) != 0)
	{
		printf("sec2Thread thread init failed, exit\n");
		return -1;
	}

	#ifdef DEBUG
		printf("MAIN Master Thread %u, waiting for kids\n", (unsigned)pthread_self());
		printf("MAIN sec1Thread: %u\n", (unsigned)sec1MasterThread);
		printf("MAIN sec2Thread: %u\n", (unsigned)sec2MasterThread);
		printf("MAIN clsThread : %u\n", (unsigned)clsMasterThread);
	#endif
	pthread_join(clsMasterThread, &clsThreadRetval);
	pthread_join(sec1MasterThread, &sec1ThreadRetval);
	pthread_join(sec2MasterThread, &sec2ThreadRetval);

	#ifdef DEBUG
	printf("all kids gone, exit\n");
	#endif

	return 0;
}
