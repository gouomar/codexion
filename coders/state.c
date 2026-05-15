#include "codexion.h"

int	is_end_program(t_rules *rules)
{
	int	res;

	pthread_mutex_lock(&rules->stop_mutex);
	res = rules->simulation_over;
	pthread_mutex_unlock(&rules->stop_mutex);
	return (res);
}

void	set_end_program(t_rules *rules, int value)
{
	pthread_mutex_lock(&rules->stop_mutex);
	if (!rules->simulation_over)
		rules->simulation_over = value;
	pthread_mutex_unlock(&rules->stop_mutex);
}
