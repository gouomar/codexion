#include "codexion.h"

static t_ll	get_priority(t_coder *coder)
{
	if (strcmp(coder->rules->scheduler, "fifo") == 0)
		return (get_time_ms());
	return (coder->last_compile + coder->rules->time_to_burnout);
}

/* Single function that handles both modes.
   The ONLY branching point is priority computation — everything else is shared. */
int take_dongle(t_coder *coder, t_dongle *dongle)
{
    t_rules *rules;

    rules = coder->rules;
    heap_insert(dongle, coder->id, get_priority(coder));
    while (!is_end_program(rules))
    {
        if (dongle->heap[0].coder_id == coder->id
            && dongle->is_taken == 0
            && dongle->available_at <= get_time_ms())
            break ;
        pthread_cond_wait(&dongle->cond, &dongle->mutex);
    }
    if (is_end_program(rules))
    {
        heap_remove_by_id(dongle, coder->id);
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

static void	dongle_order(t_coder *coder, t_dongle **first, t_dongle **second)
{
	if (coder->id % 2 == 0)
	{
		*first = coder->right_dongle;
		*second = coder->left_dongle;
	}
	else
	{
		*first = coder->left_dongle;
		*second = coder->right_dongle;
	}
}

static int	acquire_pair(t_coder *coder, t_dongle *first, t_dongle *second)
{
	t_rules	*rules;

	rules = coder->rules;
	if (!take_dongle(coder, first))
		return (0);
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

static void	compile_step(t_coder *coder, t_rules *rules)
{
	pthread_mutex_lock(&coder->state_mutex);
	coder->last_compile = get_time_ms();
	pthread_mutex_unlock(&coder->state_mutex);
	safe_print(coder, "is compiling");
	my_sleep(rules->time_to_compile);
	pthread_mutex_lock(&coder->state_mutex);
	coder->compiles_done++;
	pthread_mutex_unlock(&coder->state_mutex);
}

static int	rest_steps(t_coder *coder, t_rules *rules)
{
	if (is_end_program(rules))
		return (0);
	safe_print(coder, "is debugging");
	my_sleep(rules->time_to_debug);
	if (is_end_program(rules))
		return (0);
	safe_print(coder, "is refactoring");
	my_sleep(rules->time_to_refactor);
	return (1);
}

void	*coder_routine(void *arg)
{
	t_coder	*coder;
	t_dongle	*first;
	t_dongle	*second;
	t_rules	*rules;

	coder = (t_coder *)arg;
	rules = coder->rules;
	dongle_order(coder, &first, &second);
	while (!is_end_program(rules))
	{
		if (!acquire_pair(coder, first, second))
			break ;
		compile_step(coder, rules);
		release_dongle(first, rules->dongle_cooldown);
		release_dongle(second, rules->dongle_cooldown);
		if (!rest_steps(coder, rules))
			break ;
	}
	return (NULL);
}
