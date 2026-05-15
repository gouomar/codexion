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
	i = 0;
	finished = 0;
	while (i < ctx->rules->number_of_coders)
	{
		pthread_mutex_lock(&ctx->coders[i].state_mutex);
		if (ctx->coders[i].compiles_done
			>= ctx->rules->number_of_compiles_required)
			finished++;
		pthread_mutex_unlock(&ctx->coders[i].state_mutex);
		i++;
	}
	if (finished == ctx->rules->number_of_coders)
	{
		set_end_program(ctx->rules, 1);
		return (1);
	}
	return (0);
}

void	*monitor_routine(void *arg)
{
	t_monitor_ctx	*ctx;
	int				i;

	ctx = (t_monitor_ctx *)arg;
	while (!is_end_program(ctx->rules))
	{
		i = 0;
		while (i < ctx->rules->number_of_coders)
		{
			if (coder_burned_out(ctx, i, get_time_ms()))
				break ;
			i++;
		}
		if (all_finished_compiling(ctx))
			break ;
		usleep(1000);
	}
	free(ctx);
	return (NULL);
}
