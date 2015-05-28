#include<stdio.h>
#include<time.h>

int main(void)
{
	time_t now = time(NULL);
	printf("%d\n", now);
	return 0;
}
