#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>

// Buffer parameters
#define BUFFER_SIZE 5
#define MAX_ITEMS 10 // الحد الأقصى لعدد العناصر

int buffer[BUFFER_SIZE];
int in = 0, out = 0;
int produced_count = 0, consumed_count = 0; // عدادات للمنتج والمستهلك

// Semaphores for mutual exclusion and counting
sem_t *mutex;
sem_t *empty;
sem_t *full;

// Shared memory setup
void *create_shared_memory(size_t size) {
    int protection = PROT_READ | PROT_WRITE;
    int visibility = MAP_SHARED | MAP_ANONYMOUS;
    return mmap(NULL, size, protection, visibility, -1, 0);
}

// Function to display buffer status
void display_buffer() {
    printf("Buffer: [ ");
    for (int i = 0; i < BUFFER_SIZE; i++) {
        printf("%d ", buffer[i]);
    }
    printf("]\n");
    printf("In index: %d, Out index: %d\n", in, out);
}

// Producer function
void producer() {
    int item = 0;
    while (produced_count < MAX_ITEMS) { // التحقق من العدد الأقصى
        item++; // إنتاج عنصر
        sem_wait(empty);      // الانتظار إذا كانت الذاكرة ممتلئة
        sem_wait(mutex);      // دخول القسم الحرج

        // وضع العنصر في الذاكرة المؤقتة
        buffer[in] = item;
        printf("Producer produced: %d\n", item);
        in = (in + 1) % BUFFER_SIZE;
        produced_count++; // تحديث العداد

        // Display buffer state
        display_buffer();

        sem_post(mutex);      // الخروج من القسم الحرج
        sem_post(full);       // إشارة أن الذاكرة ليست فارغة

        sleep(rand() % 2);    // محاكاة وقت الإنتاج
    }
}

// Consumer function
void consumer() {
    while (consumed_count < MAX_ITEMS) { // التحقق من العدد الأقصى
        sem_wait(full);       // الانتظار إذا كانت الذاكرة فارغة
        sem_wait(mutex);      // دخول القسم الحرج

        int item = buffer[out]; // إزالة العنصر من الذاكرة المؤقتة
        printf("Consumer consumed: %d\n", item);
        out = (out + 1) % BUFFER_SIZE;
        consumed_count++; // تحديث العداد

        // Display buffer state
        display_buffer();

        sem_post(mutex);      // الخروج من القسم الحرج
        sem_post(empty);      // إشارة أن الذاكرة ليست ممتلئة

        sleep(rand() % 3);    // محاكاة وقت الاستهلاك
    }
}

int main() {
    // Initialize shared memory for semaphores
    mutex = create_shared_memory(sizeof(sem_t));
    empty = create_shared_memory(sizeof(sem_t));
    full = create_shared_memory(sizeof(sem_t));

    // Initialize semaphores
    sem_init(mutex, 1, 1);
    sem_init(empty, 1, BUFFER_SIZE);
    sem_init(full, 1, 0);

    // Create producer and consumer processes
    pid_t pid = fork();

    if (pid == 0) {
        // Child process: Consumer
        consumer();
    } else {
        // Parent process: Producer
        producer();
        wait(NULL); // Wait for child process to finish
    }

    // Destroy semaphores and shared memory
    sem_destroy(mutex);
    sem_destroy(empty);
    sem_destroy(full);

    return 0;
}

