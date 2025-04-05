#include "../include/mn_thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#define NUM_UTHREADS 16
#define NUM_KTHREADS 4
// #define THREADS_PER_KTHREAD (NUM_UTHREADS/NUM_KTHREADS)
#define NUM_PAGES 16


// Global array to track current thread per kernel thread
// mn_thread_t uthreads[MAX_UTHREADS];
// mn_kernel_thread_t kthreads[MAX_KTHREADS];
// ucontext_t main_context;

//nt current_thread_per_kthread[NUM_KTHREADS];

// Track remaining burst time for each thread
int remaining_time[NUM_UTHREADS];
struct itimerval timer;

// Function prototype for mn_thread_yield
void mn_thread_yield(int current_thread_index);

// Signal handler for timer
void timer_handler(int signum) {
    // Find which kernel thread is currently running
    for (int k = 0; k < NUM_KTHREADS; k++) {
        if (current_thread_per_kthread[k] != -1) {
            int thread_id = current_thread_per_kthread[k];
            printf("[TIMER] Preempting U-Thread-%d on K-Thread %d\n", thread_id, k);
            
            // Yield the current thread to the next one
            mn_thread_yield(thread_id);
        }
    }
}

void simulate_http_request(void* arg) {
    int thread_id = *(int*)arg;
    int kernel_thread_id = uthreads[thread_id].kernel_thread_id;
    int burst_remaining = BURST_TIME;

    while (burst_remaining > 0) {
        printf("[K-Thread %d] U-Thread-%d: Fetching request from page-%d (Remaining: %d)\n", 
               kernel_thread_id, thread_id, thread_id, burst_remaining);

        // Simulate work by sleeping for a short duration
        sleep(1);
        burst_remaining--;
        
        // If preemption point reached, yield voluntarily
        // Note: In a real scenario, timer would handle this automatically
        if (burst_remaining % TIME_QUANTUM == 0) {
            printf("[K-Thread %d] U-Thread-%d: Quantum expired, yielding\n", 
                   kernel_thread_id, thread_id);
            mn_thread_yield(thread_id);
        }
    }
    
    printf("[K-Thread %d] U-Thread-%d: Completed all work\n", kernel_thread_id, thread_id);
    uthreads[thread_id].state = THREAD_TERMINATED;
    mn_thread_yield(thread_id);
}

void* kernel_thread_function(void* arg) {
    mn_kernel_thread_t* kthread = (mn_kernel_thread_t*)arg;
    int k_id = kthread->id;

    //printf("Kernel Thread %d started with threads: ", k_id);
    // for (int i = 0; i <4; i++) {
    //     printf("%d ",kthread->assigned_threads[i]->id);
    // }
    // printf("\n");

    getcontext(&kthread->k_context);

    // Start the first thread in this kernel thread's group
    int first_thread_id = kthread->assigned_threads[0]->id;
    current_thread_per_kthread[k_id] = first_thread_id;
    
    printf("[K-Thread %d] Starting first U-Thread-%d\n", k_id, first_thread_id);
    uthreads[first_thread_id].state = THREAD_RUNNING;
    
    // Switch to the first thread, saving kernel thread context
    swapcontext(&kthread->k_context, &uthreads[first_thread_id].context);
    
    // Main loop - run as long as there are active threads
    while (1) {
        int all_done = 1;
        
        // Check if any threads are still active
        for (int i = 0; i < kthread->num_assigned; i++) {
            mn_thread_t* uthread = kthread->assigned_threads[i];
            if (uthread && uthread->state != THREAD_TERMINATED) {
                all_done = 0;
                break;
            }
        }
        
        if (all_done) {
            printf("[K-Thread %d] All user threads completed\n", k_id);
            break;
        }
        
        // If we get here, it means we returned from a thread that yielded
        // and need to continue the scheduling loop
        sleep(1); // Small delay to prevent CPU hogging
    }

    return NULL;
}

int main() {
    int* thread_ids[NUM_UTHREADS];
    
    // Initialize the current thread tracking array
    for(int i=0; i<NUM_KTHREADS; i++){
        current_thread_per_kthread[i] = -1;
        kthreads[i].id = i;
        kthreads[i].num_assigned =0;
        kthreads[i].current_index = 0;
    }

    printf("\n=== HTTP Request Simulation with Round-Robin M:N Threading ===\n");
    printf("Configuration: %d user threads mapped to %d kernel threads\n", 
           NUM_UTHREADS, NUM_KTHREADS);
    printf("Time Quantum: %d units, Burst Time: %d units\n\n",
           TIME_QUANTUM, BURST_TIME);

    // Initialize kernel threads and their user threads
    for (int i = 0; i < NUM_KTHREADS; i++) {
        

        // Create user threads for this kernel thread
        for (int j = 0; j < THREADS_PER_KTHREAD; j++) {
            int thread_idx = i + (j * 4);  // Fixed thread allocation pattern
            thread_ids[thread_idx] = malloc(sizeof(int));
            *thread_ids[thread_idx] = thread_idx;
            remaining_time[thread_idx] = BURST_TIME;

            // Allocate stack for the user thread
            uthreads[thread_idx].context.uc_stack.ss_sp = malloc(STACK_SIZE);
            if (uthreads[thread_idx].context.uc_stack.ss_sp == NULL) {
                perror("Failed to allocate stack for user thread");
                return -1;
            }
            uthreads[thread_idx].context.uc_stack.ss_size = STACK_SIZE;
            //uthreads[thread_idx].context.uc_stack.ss_flags = 0;
            uthreads[thread_idx].id = thread_idx;
            //uthreads[thread_idx].state = THREAD_READY;
            uthreads[thread_idx].kernel_thread_id = i;


            // Create the user thread
            if (mn_thread_create(&uthreads[thread_idx], simulate_http_request, thread_ids[thread_idx]) != 0) {
                printf("Thread creation failed\n");
                return -1;
            }

            // Assign the user thread to the current kernel thread
            kthreads[i].assigned_threads[j] = &uthreads[thread_idx];
            //printf("[K-Thread %d] Assigned U-Thread-%d at position%d\n", i, thread_idx,j);
            kthreads[i].num_assigned++;
        }

        // Create the kernel thread
        pthread_create(&kthreads[i].pthread, NULL, 
                      kernel_thread_function, &kthreads[i]);
    }

    mn_thread_map();
    printf("\n=== Starting Parallel Execution ===\n\n");

    // Wait for kernel threads to complete
    for(int i = 0; i < NUM_KTHREADS; i++) {
        pthread_join(kthreads[i].pthread, NULL);
    }

    printf("\n=== Simulation Complete ===\n");

    // Cleanup
    for(int i = 0; i < NUM_UTHREADS; i++) {
        free(thread_ids[i]);
        if (uthreads[i].context.uc_stack.ss_sp) {
            free(uthreads[i].context.uc_stack.ss_sp);
        }
    }

    return 0;
}