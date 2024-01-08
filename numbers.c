#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#define MAX_VALUE 32767  // Maximum value for 16-bit integers
#define NUM_THREADS 3 // Number of threads

// Global array and its size
int *globalArray;
int arraySize;
int steps = 0;
pthread_mutex_t steps_mutex = PTHREAD_MUTEX_INITIALIZER;

// Structure to pass arguments to the thread
typedef struct {
    int start;
    int end;
    int threadIndex;
    int (*counts)[MAX_VALUE + 1]; // Pointer to the counts array
    int fillArray; // Flag to indicate if the thread should fill the array
} ThreadArgs;

// Function prototypes
void *countingSortThread(void *args);
void aggregateCounts(int counts[][MAX_VALUE + 1], int total_counts[]);
void sortArray(int *array, int size, int total_counts[]);

// Function to display progress
void displayProgress() {
    int prev_steps = 0;
    while (1) {
        int curr_steps;
        pthread_mutex_lock(&steps_mutex);
        curr_steps = steps;
        pthread_mutex_unlock(&steps_mutex);

        if (prev_steps != curr_steps) {
            // Calculate the percentage of completion
            float percentage = (float)curr_steps / (float)arraySize * 100;
            
            // Fancy progress bar
            printf("\033[92m\r[");
            int pos = (int)(percentage / 2);
            for (int i = 0; i < 50; ++i) {
                if (i < pos) {
                    printf("\033[92m=");
                } else if (i == pos) {
                    printf("\033[93m>");
                } else {
                    printf(" ");
                }
            }
            printf("\033[92m] %.2f%%", percentage);
            
            prev_steps = curr_steps;
            fflush(stdout);
        }

        usleep(2000); // Update every 2ms
    }
}

void *fillArrayThread(void *args) {
    ThreadArgs *thread_args = (ThreadArgs *)args;

    for (int i = thread_args->start; i < thread_args->end; i++) {
        globalArray[i] = rand() % (MAX_VALUE + 1);

        pthread_mutex_lock(&steps_mutex);
        steps++;
        pthread_mutex_unlock(&steps_mutex);
    }

    return NULL;
}

void *countingSortThread(void *args) {
    ThreadArgs *thread_args = (ThreadArgs *)args;
    int threadIndex = thread_args->threadIndex;
    int (*counts)[MAX_VALUE + 1] = thread_args->counts; // Use the counts array from ThreadArgs

    // Count occurrences of each number in the thread's chunk
    for (int i = thread_args->start; i < thread_args->end; i++) {
        counts[threadIndex][globalArray[i]]++;
    }

    return NULL;
}

void aggregateCounts(int counts[][MAX_VALUE + 1], int total_counts[]) {
    for (int i = 0; i < NUM_THREADS; i++) {
        for (int j = 0; j <= MAX_VALUE; j++) {
            total_counts[j] += counts[i][j];
        }
    }
}

void sortArray(int *array, int size, int total_counts[]) {
    int index = 0;
    for (int i = 0; i <= MAX_VALUE; i++) {
        for (int j = 0; j < total_counts[i]; j++) {
            array[index++] = i;
        }
    }
}

int main() {
    // Timer variables
    struct timeval start, end, start_total, end_total;
    double time_used, total_time_used;

    // Start total timer
    gettimeofday(&start_total, NULL);

    // Thread arguments Part 1
    ThreadArgs thread_args[NUM_THREADS];

    printf("\033[92m> numbers.c 2.0\n");
    printf("\nEnter the size of the array: ");
    scanf("%d", &arraySize);

    // Allocate memory for the global array
    globalArray = (int *)malloc(arraySize * sizeof(int));
    if (globalArray == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    printf("\nFilling the array with random 16-bit integers...\n");

    // Start the display progress thread
    pthread_t progress_thread;
    pthread_create(&progress_thread, NULL, (void*)displayProgress, NULL);

    // Create threads for filling the array
    pthread_t fill_threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].fillArray = 1; // Set fillArray flag
        pthread_create(&fill_threads[i], NULL, fillArrayThread, (void *)&thread_args[i]);
    }

    // Join fill threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(fill_threads[i], NULL);
    }

    // Wait for the progress thread to finish
    pthread_cancel(progress_thread);
    pthread_join(progress_thread, NULL);

    printf("\nArray filled with random numbers.\n");

    // Create and initialize thread arguments Part 2
    int counts[NUM_THREADS][MAX_VALUE + 1] = {0}; // Counts array

    int chunk_size = arraySize / NUM_THREADS;
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].start = i * chunk_size;
        thread_args[i].end = (i == NUM_THREADS - 1) ? arraySize : (i + 1) * chunk_size;
        thread_args[i].threadIndex = i;
        thread_args[i].counts = counts; // Pass the counts array
    }

    // Create threads for counting sort
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, countingSortThread, (void *)&thread_args[i]);
    }

    // Join threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\n\033[92mCounted occurrences of each number!\n");

    // Start the timer
    gettimeofday(&start, NULL); 

    // Aggregate the counts and sort the array
    printf("\nSorting the array...\n");
    int total_counts[MAX_VALUE + 1] = {0};
    aggregateCounts(counts, total_counts);
    sortArray(globalArray, arraySize, total_counts);

    // Stop the timer
    gettimeofday(&end, NULL); 

    // Calculate the execution time
    time_used = (end.tv_sec - start.tv_sec) * 1000.0;    // sec to ms
    time_used += (end.tv_usec - start.tv_usec) / 1000.0; // us to ms
    time_used /= 1000.0;

    // Stop total timer
    gettimeofday(&end_total, NULL);

    // Calculate the total execution time
    total_time_used = (end_total.tv_sec - start_total.tv_sec) * 1000.0;    // sec to ms
    total_time_used += (end_total.tv_usec - start_total.tv_usec) / 1000.0; // us to ms
    total_time_used /= 1000.0;

    // Display the execution time
    printf("\n\033[92mSorted random numbers!\n");
    if (time_used > 1.0) {
        printf("\n\033[92m  - Execution time: %.3fs", time_used);
        printf("\n\033[92m  - Total execution time: %.3fs\n\n", total_time_used);
    } else {
        time_used *= 1000.0;
        total_time_used *= 1000.0;
        printf("\n\033[92m  - Execution time: %.3fms", time_used);
        printf("\n\033[92m  - Total execution time: %.3fms\n\n", total_time_used);
    }

    // Clean up and exit
    free(globalArray);
    return 0;
}
