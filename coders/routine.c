/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   routine.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gomar <gomar@student.1337.ma>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/15 14:32:16 by gomar             #+#    #+#             */
/*   Updated: 2026/05/15 20:06:16 by gomar            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	dongle_order(t_coder *coder, t_dongle **f, t_dongle **s)
{
	if (coder->id % 2 == 0)
	{
		*f = coder->right_dongle;
		*s = coder->left_dongle;
	}
	else
	{
		*f = coder->left_dongle;
		*s = coder->right_dongle;
	}
}

static int	acquire_pair(t_coder *coder, t_dongle *first, t_dongle *second)
{
	t_rules	*rules;

	rules = coder->rules;
	if (!take_dongle(coder, first))
		return (0);
	if (first == second)
	{
		while (!is_end_program(rules))
			usleep(500);
		release_dongle(first, rules->dongle_cooldown);
		return (0);
	}
	if (!take_dongle(coder, second))
	{
		release_dongle(first, rules->dongle_cooldown);
		return (0);
	}
	if (!is_end_program(rules))
		return (1);
	release_dongle(first, rules->dongle_cooldown);
	release_dongle(second, rules->dongle_cooldown);
	return (0);
}

static int	compile_step(t_coder *coder, t_rules *rules)
{
	pthread_mutex_lock(&coder->state_mutex);
	coder->last_compile = get_time_ms();
	pthread_mutex_unlock(&coder->state_mutex);
	safe_print(coder, "is compiling");
	if (!sleep_or_stop(coder, rules->time_to_compile))
		return (0);
	pthread_mutex_lock(&coder->state_mutex);
	coder->compiles_done++;
	pthread_mutex_unlock(&coder->state_mutex);
	return (1);
}

static int	rest_steps(t_coder *coder)
{
	if (is_end_program(coder->rules))
		return (0);
	safe_print(coder, "is debugging");
	if (!sleep_or_stop(coder, coder->rules->time_to_debug))
		return (0);
	if (is_end_program(coder->rules))
		return (0);
	safe_print(coder, "is refactoring");
	if (!sleep_or_stop(coder, coder->rules->time_to_refactor))
		return (0);
	return (1);
}

void	*coder_routine(void *arg)
{
	t_coder		*coder;
	t_dongle	*first;
	t_dongle	*second;

	coder = (t_coder *)arg;
	dongle_order(coder, &first, &second);
	while (!is_end_program(coder->rules))
	{
		if (!acquire_pair(coder, first, second))
			break ;
		if (!compile_step(coder, coder->rules))
		{
			release_dongle(first, coder->rules->dongle_cooldown);
			release_dongle(second, coder->rules->dongle_cooldown);
			break ;
		}
		release_dongle(first, coder->rules->dongle_cooldown);
		release_dongle(second, coder->rules->dongle_cooldown);
		if (!rest_steps(coder))
			break ;
	}
	return (NULL);
}
