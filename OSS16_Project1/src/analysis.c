// Put the code for your analysis program here!
#include "../include/processing_scheduling.h"
#include <stdio.h>
#include <dyn_array.h>
#include <pthread.h>
#include <string.h>

int main(int argc, char** argv) {
	printf("%s\n",argv[2]);
	if(argc < 3) {
		printf("args\n");
		return -1;
	}

	init_lock();

	dyn_array_t* ready_queue = load_process_control_blocks(argv[1]);
	if(ready_queue == (dyn_array_t*)NULL) {
		printf("no queue\n");
		return -1;
	}

	size_t n_proc = dyn_array_size(ready_queue);
	ScheduleResult_t *results = malloc(n_proc * sizeof(ScheduleResult_t));
	if(results == NULL) {
		printf("no results\n");
		dyn_array_destroy(ready_queue);
		return -1;
	}

	//argv[0] = run command, argv[1] = input filename
	int num_threads = argc-2;

	pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
	if(threads == NULL) {
		printf("no threads\n");
		free(results);
		dyn_array_destroy(ready_queue);
		return -1;
	}

	WorkerInput_t *inputs = (WorkerInput_t*)malloc(num_threads * sizeof(WorkerInput_t));
	if(inputs == NULL) {
		printf("no inputs\n");
		free(results);
		free(threads);
		dyn_array_destroy(ready_queue);
		return -1;
	}

	for(int i = 0; i < num_threads; i++) {
		inputs[i].ready_queue = ready_queue;
		inputs[i].result = &results[i];
	}

	char* fcfs = "FCFS";
	char* rr = "RR";

	for(int i = 0; i < num_threads; i++) {
		if(!strcmp(argv[2+i], fcfs)) {
			pthread_create(threads + i, NULL, first_come_first_serve_worker, (void*)(inputs + i));
		} else if(!strcmp(argv[2+i], rr)) {
			pthread_create(threads + i, NULL, round_robin_worker, (void*)(inputs + i));
		}
	}

	for(int i = 0; i < num_threads; i++) {
		pthread_join(threads[i], NULL);
	}

	for(int i = 0; i < num_threads; i++) {
		printf("%f\n", results[i].average_wall_clock_time);
	}

	free(results);
	free(threads);
	free(inputs);
	dyn_array_destroy(ready_queue);

	return 0;
}