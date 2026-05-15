/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gomar <gomar@student.1337.ma>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/15 14:30:48 by gomar             #+#    #+#             */
/*   Updated: 2026/05/15 19:57:50 by gomar            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	init_simulation(t_rules *rules)
{
	if (pthread_mutex_init(&rules->print_mutex, NULL))
		return (0);
	if (pthread_mutex_init(&rules->stop_mutex, NULL))
	{
		pthread_mutex_destroy(&rules->print_mutex);
		return (0);
	}
	rules->simulation_over = 0;
	rules->start_time = get_time_ms();
	return (1);
}

int	init_coders(t_coder *coders, t_rules *rules)
{
	int	i;

	i = 0;
	while (i < rules->number_of_coders)
	{
		coders[i].id = i + 1;
		coders[i].last_compile = rules->start_time;
		coders[i].compiles_done = 0;
		coders[i].rules = rules;
		if (pthread_mutex_init(&coders[i].state_mutex, NULL))
		{
			while (--i >= 0)
				pthread_mutex_destroy(&coders[i].state_mutex);
			return (0);
		}
		i++;
	}
	return (1);
}

int	start_monitor(t_rules *rules, t_coder *coders)
{
	pthread_t		monitor_thread;
	t_monitor_ctx	*ctx;

	ctx = malloc(sizeof(t_monitor_ctx));
	if (!ctx)
		return (0);
	ctx->coders = coders;
	ctx->rules = rules;
	if (pthread_create(&monitor_thread, NULL, monitor_routine, ctx))
	{
		free(ctx);
		return (0);
	}
	pthread_join(monitor_thread, NULL);
	return (1);
}

int	main(int argc, char **argv)
{
	t_rules		rules;
	t_coder		*coders;
	t_dongle	*dongles;

	if (!parsing(argc, argv, &rules) || !init_simulation(&rules))
		return (write(2, "Error: Invalid Arguments\n", 25), 1);
	if (rules.number_of_compiles_required == 0)
		return (write(2, "Error: Invalid Arguments\n", 25), 1);
	coders = malloc(sizeof(t_coder) * rules.number_of_coders);
	dongles = malloc(sizeof(t_dongle) * rules.number_of_coders);
	if (!coders || !dongles || !init_coders(coders, &rules))
		return (free(coders), free(dongles), 1);
	if (init_mutex(dongles, rules.number_of_coders))
		return (destroy_state_mutexes(coders, rules.number_of_coders),
			free(coders), free(dongles), 1);
	assign_dongles(coders, dongles, rules.number_of_coders);
	if (!create_threads(coders, rules.number_of_coders)
		|| !start_monitor(&rules, coders))
		set_end_program(&rules, 1);
	join_threads(coders, rules.number_of_coders);
	cleanup_all(&rules, coders, dongles);
	return (0);
}
