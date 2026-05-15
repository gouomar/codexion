#include "codexion.h"

int	create_threads(t_coder *coders, int n)
{
	int	i;

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
	int	i;

	i = 0;
	while (i < n)
	{
		if (coders[i].thread_id)
			pthread_join(coders[i].thread_id, NULL);
		i++;
	}
}

void	destroy_mutex(t_dongle *dongles, int n)
{
	int	i;

	i = 0;
	while (i < n)
	{
		pthread_mutex_destroy(&dongles[i].mutex);
		pthread_cond_destroy(&dongles[i].cond);
		free(dongles[i].heap);
		i++;
	}
}

int	init_mutex(t_dongle *dongles, int n)
{
	int	i;

	i = 0;
	while (i < n)
	{
		dongles[i].heap = malloc(sizeof(t_waiter) * n);
		if (!dongles[i].heap)
			return 0;
		if (pthread_mutex_init(&dongles[i].mutex, NULL))
		{
			free(dongles[i].heap);
			destroy_mutex(dongles, i);
			return (1);
		}
		if (pthread_cond_init(&dongles[i].cond, NULL))
		{
			pthread_mutex_destroy(&dongles[i].mutex);
			free(dongles[i].heap);
			destroy_mutex(dongles, i);
			return (1);
		}
		dongles[i].is_taken = 0;
		dongles[i].available_at = 0;
		dongles[i].heap_size = 0;
		i++;
	}
	return (0);
}

void	assign_dongles(t_coder *coders, t_dongle *dongles, int nb_coders)
{
	int	i;

	i = 0;
	while (i < nb_coders)
	{
		coders[i].left_dongle = &dongles[i];
		coders[i].right_dongle = &dongles[(i + 1) % nb_coders];
		i++;
	}
}
