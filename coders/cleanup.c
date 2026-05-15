#include "codexion.h"

void	destroy_state_mutexes(t_coder *coders, int n)
{
	int	i;

	i = 0;
	while (i < n)
	{
		pthread_mutex_destroy(&coders[i].state_mutex);
		i++;
	}
}

void	cleanup_all(t_rules *rules, t_coder *coders, t_dongle *dongles)
{
	destroy_mutex(dongles, rules->number_of_coders);
	pthread_mutex_destroy(&rules->print_mutex);
	pthread_mutex_destroy(&rules->stop_mutex);
	destroy_state_mutexes(coders, rules->number_of_coders);
	free(coders);
	free(dongles);
}
