#include "codexion.h"


long long get_time_ms(void)
{
    struct timeval  tv;

    gettimeofday(&tv, NULL);
    return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

long long my_sleep(long time)
{
	long long end;
	long long start_time;

	start_time = get_time_ms();
	end = get_time_ms() + time;
	while (get_time_ms() < end)
		usleep(500);
	return	(get_time_ms() - start_time);

}

void	safe_print(t_coder *codres, char *status)
{
	long long current_time;

	pthread_mutex_lock(&codres->rules->print_mutex);

	current_time = get_time_ms() - codres->rules->start_time;

	// Use %lld and ADD A NEWLINE \n so outputs don't mash together
	printf("%lld %d %s\n", current_time, codres->id, status);

	pthread_mutex_unlock(&codres->rules->print_mutex);
}
