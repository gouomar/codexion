/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parsing.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gomar <gomar@student.1337.ma>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/15 14:30:48 by gomar             #+#    #+#             */
/*   Updated: 2026/05/15 14:30:50 by gomar            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static long long	parse_number(char *str, int is_compile_limit)
{
	long long	res;
	int			i;

	if (!str || !str[0])
		return (-2);
	if (is_compile_limit && strcmp(str, "-1") == 0)
		return (-1);
	i = 0;
	if (str[i] == '+')
		i++;
	if (!str[i])
		return (-2);
	res = 0;
	while (str[i])
	{
		if (str[i] < '0' || str[i] > '9')
			return (-2);
		res = res * 10 + (str[i] - '0');
		if (res > 2147483647LL)
			return (-2);
		i++;
	}
	return (res);
}

static int	assign_rules(char **argv, t_rules *rules)
{
	long long	val;

	val = parse_number(argv[1], 0);
	if (val <= 0)
		return (0);
	rules->number_of_coders = val;
	rules->time_to_burnout = parse_number(argv[2], 0);
	rules->time_to_compile = parse_number(argv[3], 0);
	rules->time_to_debug = parse_number(argv[4], 0);
	rules->time_to_refactor = parse_number(argv[5], 0);
	rules->number_of_compiles_required = parse_number(argv[6], 1);
	rules->dongle_cooldown = parse_number(argv[7], 0);
	if (rules->time_to_burnout < 0 || rules->time_to_compile < 0
		|| rules->time_to_debug < 0 || rules->time_to_refactor < 0
		|| rules->dongle_cooldown < 0
		|| rules->number_of_compiles_required < -1)
		return (0);
	rules->scheduler = argv[8];
	return (1);
}

int	parsing(int argc, char **argv, t_rules *rules)
{
	if (argc != 9)
		return (0);
	if (!assign_rules(argv, rules))
		return (0);
	if (strcmp(argv[8], "fifo") != 0 && strcmp(argv[8], "edf") != 0)
		return (0);
	return (1);
}
