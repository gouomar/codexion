/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gomar <gomar@student.1337.ma>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/15 14:30:51 by gomar             #+#    #+#             */
/*   Updated: 2026/05/15 16:51:54 by gomar            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	coder_burned_out(t_monitor_ctx *ctx, int i, t_ll current_time)
{
	t_ll	last_compile;

	pthread_mutex_lock(&ctx->coders[i].state_mutex);
	last_compile = ctx->coders[i].last_compile;
	pthread_mutex_unlock(&ctx->coders[i].state_mutex);
	if (current_time - last_compile >= ctx->rules->time_to_burnout)
	{
		set_end_program(ctx->rules, 1);
		safe_print(&ctx->coders[i], "burned out");
		return (1);
	}
	return (0);
}

static int	all_finished_compiling(t_monitor_ctx *ctx)
{
	int	i;
	int	finished;

	if (ctx->rules->number_of_compiles_required == -1)
		return (0);
	if (ctx->rules->number_of_compiles_required == 0)
		return (set_end_program(ctx->rules, 1), 1);
	i = -1;
	finished = 0;
	while (++i < ctx->rules->number_of_coders)
	{
		pthread_mutex_lock(&ctx->coders[i].state_mutex);
		if (ctx->coders[i].compiles_done
			>= ctx->rules->number_of_compiles_required)
			finished++;
		pthread_mutex_unlock(&ctx->coders[i].state_mutex);
	}
	if (finished == ctx->rules->number_of_coders)
		return (set_end_program(ctx->rules, 1), 1);
	return (0);
}

static int	should_stop_from_burnout(t_monitor_ctx *ctx)
{
	int	i;

	i = 0;
	while (i < ctx->rules->number_of_coders)
	{
		if (coder_burned_out(ctx, i, get_time_ms()))
			return (1);
		i++;
	}
	return (0);
}

static void	wake_up_coders(t_monitor_ctx *ctx)
{
	int	i;

	i = 0;
	while (i < ctx->rules->number_of_coders)
	{
		pthread_mutex_lock(&ctx->coders[i].left_dongle->mutex);
		pthread_cond_broadcast(&ctx->coders[i].left_dongle->cond);
		pthread_mutex_unlock(&ctx->coders[i].left_dongle->mutex);
		i++;
	}
}

void	*monitor_routine(void *arg)
{
	t_monitor_ctx	*ctx;

	ctx = (t_monitor_ctx *)arg;
	while (!is_end_program(ctx->rules))
	{
		if (should_stop_from_burnout(ctx))
			break ;
		if (all_finished_compiling(ctx))
			break ;
		usleep(1000);
	}
	wake_up_coders(ctx);
	free(ctx);
	return (NULL);
}
