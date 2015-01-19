#include<stdio.h>
#include<pthread.h>

	pthread_mutex_t mutex;
	pthread_cond_t  condVar;

void Start()
{
	printf("new thread here: %u\n", (unsigned)pthread_self());
	while(1)
	{
		pthread_mutex_lock(&mutex);
		printf("\tthread signal %u\n", (unsigned)pthread_self());
		pthread_cond_signal(&condVar);
		pthread_mutex_unlock(&mutex);
		sleep(1);
	}
}

int main()
{
	pthread_t t1, t2, t3;

	if(pthread_cond_init(&condVar, NULL) != 0)
	{
		printf("CondVar init failed\n");
		return -1;
	}

	if(pthread_mutex_init(&mutex, NULL) != 0)
	{
		printf("mutex init failed\n");
		return -1;
	}
	// create cleartext-knx master thread
        if((pthread_create(&t1, NULL, (void *)Start, NULL)) != 0)
        {
                printf("clrThread thread init failed, exit\n");
                return -1;
	}
        if((pthread_create(&t2, NULL, (void *)Start, NULL)) != 0)
        {
                printf("clrThread thread init failed, exit\n");
                return -1;
	}
        if((pthread_create(&t3, NULL, (void *)Start, NULL)) != 0)
        {
                printf("clrThread thread init failed, exit\n");
                return -1;
        }

	while(1)
	{
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&condVar, &mutex);
		printf("got signal\n");
		pthread_mutex_unlock(&mutex);
	}

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	pthread_join(t3, NULL);

	return 0;
}
