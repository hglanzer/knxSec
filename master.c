#include "globals.h"

/*
	harald glanzer, 0727156
	harald.glanzer@gmail.com
*/

uint8_t	cls2SecBUF[CLSBUFSIZE];
uint8_t	sec2ClsBUF[SECBUFSIZE];

pthread_mutex_t cls2SecMutexWr;
pthread_mutex_t cls2SecMutexRd;
pthread_mutex_t sec2ClsMutexWr;
pthread_mutex_t sec2ClsMutexRd;

int newData4Sec = 0;
int newData4Cls = 0;

int count = 0;

void Usage(char **argv)
{
	printf("Usage: %s\n", argv[0]);
	exit(EXIT_FAILURE);
}

/*
	master prozess: creates 3 threads:
		- 2 secLine threads
		- 1 clsThreadLine threads

*/
int main(int argc, char **argv)
{
	int c, arg=3;
	
	/*
		thread - variables
	*/
	struct threadEnvCls_t threadEnvCls; 
	
	int (*clsThreadStart)(void);
	clsThreadStart = &initCls;

	int (*secStart)(void *);
	secStart = &initSec;

	void *clsThreadRetval, *sec1ThreadRetval, *sec2ThreadRetval;
	pthread_t sec1MasterThread, sec2MasterThread, clsMasterThread;
	pthread_t *sec1MasterThreadPtr, *sec2MasterThreadPtr, *clsMasterThreadPtr;

	clsMasterThreadPtr = &clsMasterThread;
	sec1MasterThreadPtr = &sec1MasterThread;
	sec2MasterThreadPtr = &sec2MasterThread;
	
	while((c = getopt(argc, argv, "hs:")) != EOF)
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
			case 's':
				threadEnvCls.socket = &optarg[0];	
			break;
			default:
				assert(0);
		}
	}
	#ifdef DEBUG
		printf("creating threads\n");
	#endif

	if(pthread_mutex_init(&cls2SecMutexWr, NULL) != 0)
	{
		printf("cls mutex init failed, exit");
		return -1;
	}
	if(pthread_mutex_init(&cls2SecMutexRd, NULL) != 0)
	{
		printf("cls mutex init failed, exit");
		return -1;
	}
	if(pthread_mutex_init(&sec2ClsMutexWr, NULL) != 0)
	{
		printf("cls mutex init failed, exit");
		return -1;
	}
	if(pthread_mutex_init(&sec2ClsMutexRd, NULL) != 0)
	{
		printf("cls mutex init failed, exit");
		return -1;
	}

	if((pthread_create(clsMasterThreadPtr, NULL, (void *)clsThreadStart, &threadEnvCls)) != 0)
	{
		printf("clsThread thread init failed, exit\n");
		return -1;
	}
	if((pthread_create(sec1MasterThreadPtr, NULL, (void *)secStart, &arg)) != 0)
	{
		printf("sec1Thread thread init failed, exit\n");
		return -1;
	}
	arg++;
	if((pthread_create(sec2MasterThreadPtr, NULL, (void *)secStart, &arg)) != 0)
	{
		printf("sec2Thread thread init failed, exit\n");
		return -1;
	}

	test("teststring");
	
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
