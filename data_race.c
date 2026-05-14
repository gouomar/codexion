#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>



void *unsafe_inc(void *arg)
{
	int *counter;

	counter = (int *) arg;
	int i = 0;
	while (i < 10000000)
	// use local variable as loop condition to make sure the data race appears

	{
		*counter += 1;
		i++;
	}
	// when u want to increament a dereferenced value use (* ptr)++;
	// cause the priority is for ++ not *
}
void *safe_inc(void *arg)
{
	pthread_mutex_t lock;
	pthread_mutex_init(&lock, NULL);

	int *count;
	count = (int *) arg;
	long long i = 0;
	while (i < 10000000	)
	{
		pthread_mutex_lock(&lock);
		*count += 1;
		pthread_mutex_unlock(&lock);
		i++;
	}
	pthread_mutex_destroy(&lock);

}


int main()
{
	pthread_t thread1, thread2;
	// init the first test with the unsafe increamenting
	int *counter;
	counter = malloc(sizeof(int));
	*counter = 0;
	pthread_create(&thread1, NULL, safe_inc, counter);
	pthread_create(&thread2, NULL, safe_inc, counter);
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	printf("%d", *counter);
	free(counter);
	return 0;

}
