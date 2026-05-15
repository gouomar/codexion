/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongles.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gomar <gomar@student.1337.ma>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/15 14:30:44 by gomar             #+#    #+#             */
/*   Updated: 2026/05/15 19:58:07 by gomar            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

t_ll	get_priority(t_coder *coder)
{
	t_ll	last;

	if (strcmp(coder->rules->scheduler, "fifo") == 0)
		return (get_time_ms());
	pthread_mutex_lock(&coder->state_mutex);
	last = coder->last_compile;
	pthread_mutex_unlock(&coder->state_mutex);
	return (last + coder->rules->time_to_burnout);
}

void	wait_on_cond(t_dongle *dongle)
{
	struct timespec	ts;

	if (dongle->available_at > 0)
	{
		ts.tv_sec = dongle->available_at / 1000;
		ts.tv_nsec = (dongle->available_at % 1000) * 1000000LL;
		pthread_cond_timedwait(&dongle->cond, &dongle->mutex, &ts);
	}
	else
		pthread_cond_wait(&dongle->cond, &dongle->mutex);
}

int	take_dongle(t_coder *coder, t_dongle *dongle)
{
	t_rules	*rules;

	rules = coder->rules;
	pthread_mutex_lock(&dongle->mutex);
	heap_insert(dongle, coder->id, get_priority(coder));
	while (!is_end_program(rules))
	{
		if (dongle->heap[0].coder_id == coder->id
			&& dongle->is_taken == 0
			&& dongle->available_at <= get_time_ms())
			break ;
		wait_on_cond(dongle);
	}
	if (is_end_program(rules))
	{
		pthread_mutex_unlock(&dongle->mutex);
		return (0);
	}
	heap_pop(dongle);
	dongle->is_taken = 1;
	pthread_mutex_unlock(&dongle->mutex);
	safe_print(coder, "has taken a dongle");
	return (1);
}

void	release_dongle(t_dongle *dongle, int coldown)
{
	pthread_mutex_lock(&dongle->mutex);
	dongle->is_taken = 0;
	dongle->available_at = get_time_ms() + coldown;
	pthread_cond_broadcast(&dongle->cond);
	pthread_mutex_unlock(&dongle->mutex);
}
