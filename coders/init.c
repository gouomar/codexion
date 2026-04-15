#include "codexion.h"


int	create_threads(t_coder *coders, int n)
{
	int i;
	i = 0;
	while (i < n)
	{
		if (pthread_create(&coders[i].thread_id, NULL, coder_routine, &coders[i]))
		{
			join_threads(coders, i);
			return (0);
		}
		i++;
	}
	return (1);
}
void	join_threads(t_coder *coders, int n)
{
	int i;

	i = 0;
	while (i < n)
	{
		pthread_join(coders[i].thread_id, NULL);
		i++;
	}
}
void destroy_mutex(t_dongle *dongle, int n)
{
	int i;
	i = 0;
	while (i < n)
	{
		pthread_mutex_destroy(&dongle[i].mutex);
		pthread_cond_destroy(&dongle[i].cond); // Destroy condition variable too
		i++;
	}
}

int init_mutex(t_dongle *dongles, int n)
{
	int i;
	i = 0;
	while (i < n)
	{
		if (pthread_mutex_init(&dongles[i].mutex, NULL))
		{
			// If one fails, destroy the ones that succeeded before returning
			destroy_mutex(dongles, i);
			return (1);
		}
		if (pthread_cond_init(&dongles[i].cond, NULL))
		{
			// Protect condition variable init too
			pthread_mutex_destroy(&dongles[i].mutex); // Destroy current mutex
			destroy_mutex(dongles, i); // Destroy earlier ones
			return (1);
		}
		dongles[i].is_taken = 0;
		dongles[i].available_at = 0;
		i++;
	}
	return (0);

}
void assigne_dongles(t_coder *coders, t_dongle *dongles, int nb_coders)
{
	int i;
	i = 0;
	while (i < nb_coders)
	{
		coders[i].left_dongle = &dongles[i];
		coders[i].right_dongle = &dongles[(i + 1) % nb_coders];
	i++;
	}
}

void *coder_routine(void *arg)
{
	t_coder *coder = (t_coder *)arg;

	// Test the new printing system!
	safe_print(coder, "is thinking");

	return NULL;
}
