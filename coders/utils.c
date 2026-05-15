/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gomar <gomar@student.1337.ma>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/15 14:32:22 by gomar             #+#    #+#             */
/*   Updated: 2026/05/15 20:19:32 by gomar            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

long long	get_time_ms(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

long long	my_sleep(long time)
{
	long long	end;
	long long	start_time;

	start_time = get_time_ms();
	end = get_time_ms() + time;
	while (get_time_ms() < end)
		usleep(500);
	return (get_time_ms() - start_time);
}

void	safe_print(t_coder *codres, char *status)
{
	long long	current_time;

	pthread_mutex_lock(&codres->rules->print_mutex);
	if (is_end_program(codres->rules) && strcmp(status, "burned out") != 0)
	{
		pthread_mutex_unlock(&codres->rules->print_mutex);
		return ;
	}
	current_time = get_time_ms() - codres->rules->start_time;
	printf("%lld %d %s\n", current_time, codres->id, status);
	pthread_mutex_unlock(&codres->rules->print_mutex);
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

int	sleep_or_stop(t_coder *coder, int duration_ms)
{
	long long	end;

	end = get_time_ms() + duration_ms;
	while (!is_end_program(coder->rules) && get_time_ms() < end)
		usleep(500);
	return (!is_end_program(coder->rules));
}
