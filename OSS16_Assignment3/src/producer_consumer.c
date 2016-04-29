#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include <fcntl.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>

#define DATA_SIZE 256
#define BUFF_SIZE 4096

int validate_consumer(const char *ground_truth, const char *consumer_buffer);

int main(void) {
    // seed the random number generator
    srand(time(NULL));

    // Parent and Ground Truth Buffers
    char ground_truth[BUFF_SIZE]    = {0};  // used to verify
    char producer_buffer[BUFF_SIZE] = {0};  // used by the parent

    // init the ground truth and parent buffer
    for (int i = 0; i < BUFF_SIZE; ++i) {
        producer_buffer[i] = ground_truth[i] = rand() % 256;
    }

    // System V IPC keys for you to use
    // const key_t s_msq_key = 1337;  // used to create message queue ipc
    // const key_t s_shm_key = 1338;  // used to create shared memory ipc
    // const key_t s_sem_key = 1339;  // used to create semaphore ipc
    // POSIX IPC keys for you to use
    const char *const p_msq_key = "/OS_MSG";
    const char *const p_shm_key = "/OS_SHM";
    const char *const p_sem_key = "/OS_SEM_FULL";
    const char *const p_sem_key2 = "/OS_SEM_EMPTY";

    /*
    * MESSAGE QUEUE SECTION
    **/
    mqd_t msgq_id;
    //open the message queue
    struct mq_attr attr = {0,10,DATA_SIZE,0,{0}};
    msgq_id = mq_open(p_msq_key, O_RDWR | O_CREAT | O_EXCL, S_IRWXU | S_IRWXG, &attr);
    if(msgq_id == (mqd_t)-1) {
        //originally just error'd here
        //HOWEVER, if an error happens somewhere else and the message queue doesn't get unlinked (later)
        //then the message queue may exist already
        //so try to open an existing queue
        msgq_id = mq_open(p_msq_key, O_RDWR);
        if(msgq_id == (mqd_t)-1) {
            //now we really have an error because we couldn't create or open existing message queue
            perror("in mq_open()");
            return -1;
        }
    } else {
        //fork and do stuff
        int pid = fork();
        if(pid == -1) {
            perror("in fork()");
            return -1;
        } else if(pid == 0) {
            //child process
            char consumer_buffer[BUFF_SIZE] = {0};
            //we're going to loop to receive msgs...
            //i guess this is ok since we know exactly how many messages we're waiting for...?
            for(int i = 0; i < BUFF_SIZE/DATA_SIZE; i++) {
                //receive a message (blocks until message is available)
                mq_receive(msgq_id, consumer_buffer+(i*DATA_SIZE), DATA_SIZE, NULL);
            }
            //validate 4K transfer (validate_consumer returns 0 on success >0 on failure)
            int test = validate_consumer(ground_truth, consumer_buffer);
            if(!test) {
                printf("Message Queue Transfer Valid\n");
            } else {
                printf("Message Queue Transfer Invalid at Byte %d\n", test);
            }
            //child exit
            _exit(EXIT_SUCCESS);
        } else {
            //parent process
            for(int i = 0; i < BUFF_SIZE/DATA_SIZE; i++) {
                //loops to send all the messages (DATA_SIZE chunks of buffer)
                mq_send(msgq_id, producer_buffer+(i*DATA_SIZE), DATA_SIZE, 0);
            }
            //wait for child
            waitpid(pid, NULL, 0);
        }
    }
    //close and unlink message queue
    mq_close(msgq_id);
    mq_unlink(p_msq_key);

    /*
    * PIPE SECTION
    **/
    // man 7 pipe: reads and writes less than 4096 bytes are atomic
    int pfd[2];
    if(pipe(pfd) == -1) {
        return -1;
    } else {
        int pid = fork();
        if(pid == -1) {
            perror("in fork()");
            return -1;
        } else if(pid == 0) {
            //child process
            close(pfd[1]); //close the write end
            char consumer_buffer[BUFF_SIZE] = {0};
            int totalread = 0;
            int numread;
            for(;;) { //the pipe will tell us when we're done
                //read from pipe to consumer_buffer (blocks until pipe has data)
                numread = read(pfd[0], &consumer_buffer[totalread], DATA_SIZE);
                if(numread == 0) { //parent closed write end of pipe and read returned 0 for EOF signal
                    break; //get out of infinite for loop
                }
                totalread += numread; //add to total number of bytes read
            }

            close(pfd[0]); //now close the read end since we're done
            //validate 4K transfer (validate_consumer returns 0 on success >0 on failure)
            int test = validate_consumer(ground_truth, consumer_buffer);
            if(!test) {
                printf("Pipe Transfer Valid\n");
            } else {
                printf("Pipe Transfer Invalid at Byte %d\n", test);
            }
            //child exit
            _exit(EXIT_SUCCESS);
        } else {
            //parent process
            close(pfd[0]); //close the read end
            for(int i = 0; i < BUFF_SIZE/DATA_SIZE; i++) {
                //write to pipe from producer_buffer (blocks if pipe full, until pipe has room)
                write(pfd[1], producer_buffer+(i*DATA_SIZE), DATA_SIZE);
            }
            close(pfd[1]); //close the write end (signals child EOF by read returning 0 instead of blocking)
            //wait for child
            waitpid(pid, NULL, 0);
        }
    }

    /*
    * SHARED MEMORY AND SEMAPHORE SECTION
    **/
    int sem_full_id, sem_empty_id, shm_fd;
    void *ptr;
    sem_t *sem_full, *sem_empty;

    //open shared memory segment for buffer
    if((shm_fd = shm_open(p_shm_key, O_CREAT | O_RDWR, 0666)) == -1) {
        perror("buff shm_open()");
        return -1;
    }
    //truncate size to DATA_SIZE
    if(ftruncate(shm_fd, DATA_SIZE) == -1) {
        perror("buff ftruncate()");
        return -1;
    }
    //map into memory
    if((ptr = mmap(0, DATA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED) {
        perror("buff mmap()");
        return -1;
    }
    //open, set size and map shared memory segment for sem_full semaphore
    if((sem_full_id = shm_open(p_sem_key, O_CREAT | O_RDWR, 0666)) == -1) {
        perror("sem_full shm_open()");
        return -1;
    }
    if(ftruncate(sem_full_id, sizeof(sem_t)) == -1) {
        perror("sem_full ftruncate()");
        return -1;
    }
    if((sem_full = mmap(0, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, sem_full_id, 0)) == MAP_FAILED) {
        perror("sem_full sem mmap()");
        return -1;
    }
    //initialize sem_full (to 0 because the buffer is empty to start with)
    if(sem_init(sem_full, 1, 0) == -1) {
        perror("sem_full sem_init()");
        return -1;
    }
    //open, set size and map shared memory segment for sem_empty semaphore
    if((sem_empty_id = shm_open(p_sem_key2, O_CREAT | O_RDWR, 0666)) == -1) {
        perror("sem_empty shm_open()");
        return -1;
    }
    if(ftruncate(sem_empty_id, sizeof(sem_t)) == -1) {
        perror("sem_empty ftruncate()");
        return -1;
    }
    if((sem_empty = mmap(0, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, sem_empty_id, 0)) == MAP_FAILED) {
        perror("sem_empty mmap()");
        return -1;
    }
    //initialize sem_empty (to 1 because the buffer is empty to start with)
    if(sem_init(sem_empty, 1, 1) == -1) {
        perror("sem_empty - sem_init()");
        return -1;
    }

    //fork and do stuff

    int pid = fork();
    if(pid == -1) {
        perror("fork()");
        return -1;
    } else if(pid == 0) {
        //child process
        char consumer_buffer[BUFF_SIZE] = {0};
        int j;

        for(j = 0; j < BUFF_SIZE/DATA_SIZE; j++) {
            sem_wait(sem_full); //waits for sem_full (parent releases after write is completed)

            memcpy(consumer_buffer+(j*DATA_SIZE), ptr, DATA_SIZE); //copy data from SHM to consumer_buffer

            sem_post(sem_empty); //releases sem_empty (parent can now lock sem_empty to write another chunk)
        }

        //validate 4K transfer (validate_consumer returns 0 on success >0 on failure)
        int test = validate_consumer(ground_truth, consumer_buffer);
        if(!test) {
            printf("Shared Memory Valid\n");
        } else {
            printf("Shared Memory Invalid at Byte %d\n", test);
        }
        //child exit
        _exit(EXIT_SUCCESS);
    } else {
        //parent process
        int j;
        for(j = 0; j < BUFF_SIZE/DATA_SIZE; j++) {
            sem_wait(sem_empty); //waits for sem_empty (child releases after read is completed)

            memcpy(ptr, producer_buffer+(j*DATA_SIZE), DATA_SIZE); //copy data to SHM from producer_buffer

            sem_post(sem_full); //releases sem_full (child can now lock sem_full to read chunk)
        }
        //wait for child
        waitpid(pid, NULL, 0);
    }

    //and we're back to one process

    //destroy semaphores
    sem_destroy(sem_full);
    sem_destroy(sem_empty);
    //unmap shared memory segments
    munmap(ptr, DATA_SIZE);
    munmap(sem_full, sizeof(sem_t));
    munmap(sem_empty, sizeof(sem_t));
    //unlink shared memory segments
    shm_unlink(p_sem_key);
    shm_unlink(p_sem_key2);
    shm_unlink(p_shm_key);

    //congratulations
    return 0;
}

//this is pretty self-explanatory helper function...
//if you don't understand, you're in the wrong place.
int validate_consumer(const char *ground_truth, const char *consumer_buffer) {
    for(int i = 0; i < BUFF_SIZE; i++) {
        if(consumer_buffer[i] != ground_truth[i]) {
            return i+1;
        }
    }

    return 0;
}
