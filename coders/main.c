#include "codexion.h"







int main(int argc, char **argv)
{
    t_rules rules; // Stack allocation
    t_coder *coders;
	t_dongle *dongles;
    int i;

    // 1. Parse arguments (pass the address of rules)
    if (!parsing(argc, argv, &rules))
    {
        write(2, "Error: Invalid Arguments\n", 25);
        return (1);
    }
	// init the mutex for the safe print and for monitor!
	pthread_mutex_init(&rules.print_mutex, NULL);
	pthread_mutex_init(&rules.stop_mutex, NULL);
	// Start the Master Stopwatch just before setting up array!
	rules.start_time = get_time_ms();

	// create the thread for monitoring
	pthread_create(&coders[i].thread_id, NULL, coder_routine, &coders[i])
    // 2. Allocate one flat array for all coders
    coders = malloc(sizeof(t_coder) * rules.number_of_coders);
    if (!coders)
    {
        printf("Error: no available memory on the heap\n");
        return (1);
    }
	dongles = malloc(sizeof(t_dongle) * rules.number_of_coders);
	if (!dongles)
	{
        printf("Error: no available memory on the heap\n");
		return (1);
	}

    i = 0;
    while (i < rules.number_of_coders)
    {
        coders[i].id = i + 1;        // Use '.' instead of '->' we only work on stack
        coders[i].rules = &rules;
		    // Point back to the master rules
        i++;                         //
    }
	init_mutex(dongles, rules.number_of_coders);

	// ASSIGN DONGLES BEFORE CREATING THREADS!
	assigne_dongles(coders, dongles, rules.number_of_coders);

	if (!create_threads(coders, rules.number_of_coders))
	{
		// Clean up everything if thread creation fails
		destroy_mutex(dongles, rules.number_of_coders);
		free(coders);
		free(dongles);
		return (1);
	}

	join_threads(coders, rules.number_of_coders);

	// DESTROY MUTEXES AFTER ALL THREADS ARE FINISHED
	destroy_mutex(dongles, rules.number_of_coders);
	pthread_mutex_destroy(&rules.print_mutex);

    free(coders);
	free(dongles);
    return (0);
}
