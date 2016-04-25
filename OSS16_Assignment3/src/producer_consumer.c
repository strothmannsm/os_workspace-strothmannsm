#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#define DATA_SIZE 256
#define BUFF_SIZE 4098


int main(void) {
    // seed the random number generator
    srand(time(NULL));

    // Parent and Ground Truth Buffers
    char ground_truth[BUFF_SIZE]    = {0};  // used to verify
    char producer_buffer[BUFF_SIZE] = {0};  // used by the parent

    // init the ground truth and parent buffer
    for (int = 0; i < BUFF_SIZE; ++i) {
        producer_buffer[i] = ground_truth[i] = rand() % 256;
    }

    // System V IPC keys for you to use
    const key_t s_msq_key = 1337;  // used to create message queue ipc
    const key_t s_shm_key = 1338;  // used to create shared memory ipc
    const key_t s_sem_key = 1339;  // used to create semaphore ipc
    // POSIX IPC keys for you to use
    const char *const p_msq_key = "OS_MSG";
    const char *const p_shm_key = "OS_SHM";
    const char *const p_sem_key = "OS_SEM";

    /*
    * MESSAGE QUEUE SECTION
    **/

    /*
    * PIPE SECTION
    **/

    /*
    * SHARED MEMORY AND SEMAPHORE SECTION
    **/

    return 0;
}
