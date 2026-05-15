#include "codexion.h"

static int	is_valid_number(char *str)
{
	int	i;

	if (!str || !str[0])
		return (0);
	i = 0;
	if (str[i] == '+')
		i++;
	if (!str[i])
		return (0);
	while (str[i])
	{
		if (str[i] < '0' || str[i] > '9')
			return (0);
		i++;
	}
	return (1);
}

int	parsing(int argc, char **argv, t_rules *rules)
{
	int	i;

	if (argc != 9)
		return (0);
	i = 1;
	while (i <= 7)
	{
		if (!is_valid_number(argv[i]))
			return (0);
		i++;
	}
	if (strcmp(argv[8], "fifo") != 0 && strcmp(argv[8], "edf") != 0)
		return (0);
	rules->number_of_coders = atoi(argv[1]);
	rules->time_to_burnout = atoi(argv[2]);
	rules->time_to_compile = atoi(argv[3]);
	rules->time_to_debug = atoi(argv[4]);
	rules->time_to_refactor = atoi(argv[5]);
	rules->number_of_compiles_required = atoi(argv[6]);
	rules->dongle_cooldown = atoi(argv[7]);
	rules->scheduler = argv[8];
	return (1);
}
