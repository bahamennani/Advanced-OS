#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 5  // Fixed size of the buffer

// Circular buffer structure
typedef struct {
    int buffer[BUFFER_SIZE];
    int in;         // Index for the next write
    int out;        // Index for the next read
    int count;      // Number of items in the buffer
    pthread_mutex_t mutex;         // Mutex for synchronizing access
    pthread_cond_t not_empty;      // Condition variable for non-empty buffer
    pthread_cond_t not_full;       // Condition variable for non-full buffer
} Buffer;

// Initialize the buffer
void init_buffer(Buffer *buf) {
    buf->in = 0;
    buf->out = 0;
    buf->count = 0;
    pthread_mutex_init(&buf->mutex, NULL);
    pthread_cond_init(&buf->not_empty, NULL);
    pthread_cond_init(&buf->not_full, NULL);
}

// Clean up buffer resources
void destroy_buffer(Buffer *buf) {
    pthread_mutex_destroy(&buf->mutex);
    pthread_cond_destroy(&buf->not_empty);
    pthread_cond_destroy(&buf->not_full);
}

// Producer function
void *producer(void *param) {
    Buffer *buf = (Buffer *)param;
    for (int i = 0; i < 10; i++) {  // Producing 10 items as example
        pthread_mutex_lock(&buf->mutex);  // Lock the buffer

        while (buf->count == BUFFER_SIZE) {  // Wait if buffer is full
            pthread_cond_wait(&buf->not_full, &buf->mutex);
        }

        // Produce an item
        buf->buffer[buf->in] = i;
        printf("Produced: %d\n", i);
        buf->in = (buf->in + 1) % BUFFER_SIZE;
        buf->count++;

        pthread_cond_signal(&buf->not_empty);  // Signal consumer that buffer is not empty
        pthread_mutex_unlock(&buf->mutex);     // Unlock the buffer

        sleep(1);  // Simulate production time
    }
    pthread_exit(0);
}

// Consumer function
void *consumer(void *param) {
    Buffer *buf = (Buffer *)param;
    for (int i = 0; i < 10; i++) {  // Consuming 10 items as example
        pthread_mutex_lock(&buf->mutex);  // Lock the buffer

        while (buf->count == 0) {  // Wait if buffer is empty
            pthread_cond_wait(&buf->not_empty, &buf->mutex);
        }

        // Consume an item
        int item = buf->buffer[buf->out];
        printf("Consumed: %d\n", item);
        buf->out = (buf->out + 1) % BUFFER_SIZE;
        buf->count--;

        pthread_cond_signal(&buf->not_full);  // Signal producer that buffer has space
        pthread_mutex_unlock(&buf->mutex);    // Unlock the buffer

        sleep(1);  // Simulate consumption time
    }
    pthread_exit(0);
}

// Main function to test the implementation
int main() {
    Buffer buf;
    init_buffer(&buf);

    pthread_t producer_thread, consumer_thread;

    // Create producer and consumer threads
    pthread_create(&producer_thread, NULL, producer, &buf);
    pthread_create(&consumer_thread, NULL, consumer, &buf);

    // Wait for both threads to complete
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    // Clean up resources
    destroy_buffer(&buf);

    return 0;
}
