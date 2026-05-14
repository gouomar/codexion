#ifndef CODEXION_H
# define CODEXION_H


# include <pthread.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/time.h>
# include <unistd.h>

typedef struct s_dongle	t_dongle;
typedef long long		t_ll;
typedef pthread_mutex_t	t_mutex;
typedef pthread_cond_t	t_cond;
typedef struct s_waiter t_waiter;

typedef struct s_rules
{
	int			number_of_coders;
	int			time_to_burnout;
	int			time_to_compile;
	int			time_to_debug;
	int			time_to_refactor;
	int			number_of_compiles_required;
	int			dongle_cooldown;
	t_ll		start_time;
	char		*scheduler;
	t_mutex		print_mutex;
	int			simulation_over;
	t_mutex		stop_mutex;
}t_rules;

typedef struct s_coder
{
	int			id;
	pthread_t	thread_id;
	t_dongle	*left_dongle;
	t_dongle	*right_dongle;
	t_ll		last_compile;
	int			compiles_done;
	t_mutex		state_mutex;
	t_rules		*rules;
}t_coder;

typedef struct s_monitor_ctx
{
	t_coder	*coders;
	t_rules	*rules;
}t_monitor_ctx;

struct s_dongle
{
	t_mutex		mutex;
	t_cond		cond;
	int			is_taken;
	t_ll		available_at;
	t_waiter	*heap;
	int			heap_size;
};

typedef struct s_waiter
{
	int		coder_id; // who is wating (index in the mini-heap)
	t_ll	priority; // arrival_time (FIFO) or deadline (EDF)
}t_waiter;


int				parsing(int argc, char **argv, t_rules *rules);
void			*coder_routine(void *arg);
int				create_threads(t_coder *coders, int n);
void			join_threads(t_coder *coders, int n);
void	destroy_state_mutexes(t_coder *coders, t_dongle *dongles, int n);
void			cleanup_all(t_rules *rules, t_coder *coders, t_dongle *dongles);
int				init_mutex(t_dongle *dongles, int n);
void			destroy_mutex(t_dongle *dongles, int n);
void			assign_dongles(t_coder *c, t_dongle *d, int n);
long long		get_time_ms(void);
long long		my_sleep(long time);
void			safe_print(t_coder *coder, char *status);
int				is_end_program(t_rules *rules);
void			set_end_program(t_rules *rules, int value);
void			*monitor_routine(void *arg);
void			heap_insert(t_dongle *d, int coder_id, t_ll priority);
void			heap_pop(t_dongle *d);
void			heap_remove_by_id(t_dongle *d, int coder_id);

#endif
