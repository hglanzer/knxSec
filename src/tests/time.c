#include<stdio.h>
#include<time.h>

int main(void)
{
	time_t now = time(NULL);
	char buf[4];
	int i=0;
	printf("%10d = 0x%x, size = %d\n", now, now, sizeof(now));

	printf("             0x");
	for(i=0; i<4;i++)
	{
		buf[i] = now % 256;
		now = now / 256;
	}
	for(i=3; i>=0;i=i-1)
	{
		printf("%x", buf[i]);
	}
	printf("\n\n");
	return 0;
}
