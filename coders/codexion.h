#ifndef CODEXION_H
# define CODEXION_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h> // Needed for pthread_t, pthread_mutex_t, pthread_cond_t
#include <unistd.h>
#include <sys/time.h>

typedef struct s_rules {
    int number_of_coders;
    int time_to_burnout;
    int time_to_compile;
    int time_to_debug;
    int time_to_refactor;
    int number_of_compiles_required;
    int dongle_cooldown;
	long long start_time;
    char *scheduler;
	pthread_mutex_t print_mutex;
	int simulation_over;
	pthread_mutex_t stop_mutex;
} t_rules;

// Forward declaration of t_dongle so t_coder can use it as a pointer
typedef struct s_dongle t_dongle;

typedef struct s_coder
{
	int id;                     // The coder's number (1 to N)
	pthread_t thread_id;        // The POSIX thread representing this coder
	t_dongle *left_dongle;      // Pointer to the exact left dongle struct
	t_dongle *right_dongle;     // Pointer to the exact right dongle struct
	long long last_compile;     // Better as long long (for large ms timestamps), NOT a pointer
	int compiles_done;          // Just a normal integer counter, NOT a pointer
	t_rules *rules;             // Pointer to the master rules
} t_coder;

struct s_dongle
{
	pthread_mutex_t mutex;      // The physical lock for this specific dongle
	pthread_cond_t cond;        // The wait queue for coders wanting this dongle
	int is_taken;               // Flag: 0 if available, 1 if someone is using it
	long long available_at;     // Timestamp of when the cooldown finishes
};

int parsing(int argc, char **argv, t_rules *rules);
void *coder_routine(void *coders);
int create_threads(t_coder *coders, int n);
void join_threads(t_coder *coders, int n);
int init_mutex(t_dongle *dongles, int n);
void destroy_mutex(t_dongle *dongle, int n);
void assigne_dongles(t_coder *coders, t_dongle *dongles, int nb_coders);
long long get_time_ms(void);
long long my_sleep(long time);
void safe_print(t_coder *codres, char *status);






#endif
