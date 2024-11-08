#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define BUFFER_SIZE 5  // Fixed size of the buffer
#define NUM_PRODUCERS 3
#define NUM_CONSUMERS 3

// Structure for circular buffer in shared memory
typedef struct {
    int buffer[BUFFER_SIZE];
    int in;     // Index for the next write
    int out;    // Index for the next read
    int count;  // Number of items in the buffer
} SharedBuffer;

// Semaphore controls
enum {MUTEX, EMPTY, FULL};  // Semaphore indexes

// Utility function for semaphore operations
void sem_op(int sem_id, int sem_num, int op) {
    struct sembuf operation = {sem_num, op, 0};
    semop(sem_id, &operation, 1);
}

// Producer function
void producer(int sem_id, SharedBuffer *shared_buffer, int producer_id) {
    for (int i = 0; i < 10; i++) {
        sem_op(sem_id, EMPTY, -1);    // Wait if buffer is full
        sem_op(sem_id, MUTEX, -1);    // Lock the buffer

        // Produce an item
        shared_buffer->buffer[shared_buffer->in] = i;
        printf("Producer %d produced: %d\n", producer_id, i);
        shared_buffer->in = (shared_buffer->in + 1) % BUFFER_SIZE;
        shared_buffer->count++;

        sem_op(sem_id, MUTEX, 1);     // Unlock the buffer
        sem_op(sem_id, FULL, 1);      // Signal that buffer has an item

        sleep(1);  // Simulate production time
    }
}

// Consumer function
void consumer(int sem_id, SharedBuffer *shared_buffer, int consumer_id) {
    for (int i = 0; i < 10; i++) {
        sem_op(sem_id, FULL, -1);     // Wait if buffer is empty
        sem_op(sem_id, MUTEX, -1);    // Lock the buffer

        // Consume an item
        int item = shared_buffer->buffer[shared_buffer->out];
        printf("Consumer %d consumed: %d\n", consumer_id, item);
        shared_buffer->out = (shared_buffer->out + 1) % BUFFER_SIZE;
        shared_buffer->count--;

        sem_op(sem_id, MUTEX, 1);     // Unlock the buffer
        sem_op(sem_id, EMPTY, 1);     // Signal that buffer has space

        sleep(1);  // Simulate consumption time
    }
}

// Main function
int main() {
    // Create shared memory segment for buffer
    int shm_id = shmget(IPC_PRIVATE, sizeof(SharedBuffer), IPC_CREAT | 0666);
    SharedBuffer *shared_buffer = (SharedBuffer *)shmat(shm_id, NULL, 0);

    // Initialize shared buffer
    shared_buffer->in = 0;
    shared_buffer->out = 0;
    shared_buffer->count = 0;

    // Create semaphore set
    int sem_id = semget(IPC_PRIVATE, 3, IPC_CREAT | 0666);
    semctl(sem_id, MUTEX, SETVAL, 1);  // Mutex semaphore
    semctl(sem_id, EMPTY, SETVAL, BUFFER_SIZE);  // Empty slots in buffer
    semctl(sem_id, FULL, SETVAL, 0);   // Full slots in buffer

    // Create producer processes
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        if (fork() == 0) {
            producer(sem_id, shared_buffer, i);
            exit(0);
        }
    }

    // Create consumer processes
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        if (fork() == 0) {
            consumer(sem_id, shared_buffer, i);
            exit(0);
        }
    }

    // Wait for all child processes to finish
    for (int i = 0; i < NUM_PRODUCERS + NUM_CONSUMERS; i++) {
        wait(NULL);
    }

    // Cleanup
    shmdt(shared_buffer);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);

    return 0;
}
