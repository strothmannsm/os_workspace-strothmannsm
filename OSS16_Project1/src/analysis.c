// Put the code for your analysis program here!
#include "../include/processing_scheduling.h"
#include <stdio.h>
#include <dyn_array.h>
#include <pthread.h>
#include <string.h>

int main(int argc, char** argv) {
	// check args
	if(argc < 3) {
		printf("Insufficient Arguments\n"
			"Correct Usage: %s <input filename> <FCFS/RR> ..\n"
			"Must request at least one FCFS or RR core.\n", argv[0]);
		return -1;
	}

	init_lock();

	// load PCBs from input file
	dyn_array_t* ready_queue = load_process_control_blocks(argv[1]);
	if(ready_queue == (dyn_array_t*)NULL) {
		printf("PCBs unable to load\n");
		return -1;
	}

	size_t n_proc = dyn_array_size(ready_queue);

	// allocate array of ScheduleResult_t structures
	ScheduleResult_t *results = malloc(n_proc * sizeof(ScheduleResult_t));
	if(results == NULL) {
		printf("Memory allocation failure: Result Structures\n");
		dyn_array_destroy(ready_queue);
		return -1;
	}

	//argv[0] = run command, argv[1] = input filename
	int num_threads = argc-2;

	// allocate array of pthread_t thread structures
	pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
	if(threads == NULL) {
		printf("Memory allocation failure: Threads\n");
		free(results);
		dyn_array_destroy(ready_queue);
		return -1;
	}

	// allocate array of WorkerInput_t structures
	WorkerInput_t *inputs = (WorkerInput_t*)malloc(num_threads * sizeof(WorkerInput_t));
	if(inputs == NULL) {
		printf("Memory allocation failure: WorkerInput Structures\n");
		free(results);
		free(threads);
		dyn_array_destroy(ready_queue);
		return -1;
	}

	// load WorkerInput_t structures
	// each thread shares ready_queue, but has it's own ScheduleResult_t structure
	for(int i = 0; i < num_threads; i++) {
		inputs[i].ready_queue = ready_queue;
		inputs[i].result = &results[i];
	}

	// create strings for FCFS and RR to use strcmp to determing which which scheduler to send each thread to
	char* fcfs = "FCFS";
	char* rr = "RR";

	// create all the threads
	for(int i = 0; i < num_threads; i++) {
		int error;
		if(!strcmp(argv[2+i], fcfs)) {
			pthread_create(threads + i, NULL, first_come_first_serve_worker, (void*)(inputs + i));
		} else if(!strcmp(argv[2+i], rr)) {
			pthread_create(threads + i, NULL, round_robin_worker, (void*)(inputs + i));
		} else {
			printf("%s is not a valid core request.  Please use \"FCFS\" and/or \"RR\" in all caps\n", argv[2+i]);
		}
	}

	// join threads
	for(int i = 0; i < num_threads; i++) {
		if(pthread_join(threads[i], NULL)) {
			printf("Join error\n");
		}
	}

	// free all memory
	free(results);
	free(threads);
	free(inputs);
	dyn_array_destroy(ready_queue);

	// successful exectution
	return 0;
}