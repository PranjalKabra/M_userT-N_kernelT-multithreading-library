#include "../include/mn_thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_PAGES 16

void simulate_http_request(void* arg) {
    int thread_id = *(int*)arg;
    int kernel_thread_id = uthreads[thread_id].kernel_thread_id;
    int burst_remaining = burst_time;
    int x = burst_time % time_quantum;

    while (burst_remaining > 0) {
        printf("[K-Thread %d] U-Thread-%d: Fetching request from page-%d (Remaining: %d)\n", kernel_thread_id, thread_id, thread_id, burst_remaining);

        // simulation of work
        sleep(1);
        burst_remaining--;
        if (burst_remaining % time_quantum == x) {
            printf("[K-Thread %d] U-Thread-%d: Quantum expired, yielding\n",  kernel_thread_id, thread_id);
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
    getcontext(&kthread->k_context);

    //when the first thread is starting, the first thread id will be same as kernel thread id k_id
    int first_thread_id = kthread->assigned_threads[0]->id;
    current_thread_per_kthread[k_id] = first_thread_id;
    
    printf("[K-Thread %d] Starting first U-Thread-%d\n", k_id, first_thread_id);
    uthreads[first_thread_id].state = THREAD_RUNNING;
    
    // Switch to the first thread, saving kernel thread context
    swapcontext(&kthread->k_context, &uthreads[first_thread_id].context);
    
    // run loop as long as any thread is active
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
        
        // If we get here, it means we returned from a thread that hasn't terminated
        // and need to continue the scheduling loop
        sleep(1); // Small delay
    }

    return NULL;
}

int main() {
    mn_thread_init(16, 4, 5, 12); // 16 uthreads, 4 kthreads, quantum 5, burst 12
    int* thread_ids[num_uthreads];

    
    // Initialization of k thread array
    for(int i=0; i<num_kthreads; i++){
        current_thread_per_kthread[i] = -1;
        kthreads[i].id = i;
        kthreads[i].num_assigned =0;
        kthreads[i].current_index = 0;
    }

    printf("\n=== HTTP Request Simulation with Round-Robin M:N Threading ===\n");
    printf("Configuration: %d user threads mapped to %d kernel threads\n", 
        num_uthreads, num_kthreads);
    printf("Time Quantum: %d units, Burst Time: %d units\n\n",
           time_quantum, burst_time);

    // initializationf of k threads and assign their respective user threads
    for (int i = 0; i < num_kthreads; i++) {
        
        for (int j = 0; j < threads_per_kthread; j++) {
            int thread_idx = i + (j * 4); 
            thread_ids[thread_idx] = malloc(sizeof(int));
            *thread_ids[thread_idx] = thread_idx;

            // Allocate stack per user thread
            uthreads[thread_idx].context.uc_stack.ss_sp = malloc(STACK_SIZE); // set the stack ponter
            if (uthreads[thread_idx].context.uc_stack.ss_sp == NULL) {
                perror("Failed to allocate stack for user thread");
                return -1;
            }

            // set the thread identitiy and allocate it it's kernel thread before starting its routine
            uthreads[thread_idx].id = thread_idx;
            uthreads[thread_idx].kernel_thread_id = i;


            // Create the user thread -> allocate stack, set context and set state and start routine
            if (mn_thread_create(&uthreads[thread_idx], simulate_http_request, thread_ids[thread_idx]) != 0) {
                printf("Thread creation failed\n");
                return -1;
            }

            kthreads[i].assigned_threads[j] = &uthreads[thread_idx];
            kthreads[i].num_assigned++;
        }

        // create the kernel thread
        pthread_create(&kthreads[i].pthread, NULL, kernel_thread_function, &kthreads[i]);
    }

    mn_thread_map();
    printf("\n=== Starting Parallel Execution ===\n\n");

    // wait for all 4 kernel threads to complete
    for(int i = 0; i < num_kthreads; i++) {
        pthread_join(kthreads[i].pthread, NULL);
    }

    printf("\n=== Simulation Complete ===\n");

    // Cleanup
    for(int i = 0; i < num_uthreads; i++) {
        free(thread_ids[i]);
        if (uthreads[i].context.uc_stack.ss_sp) {
            free(uthreads[i].context.uc_stack.ss_sp);
        }
    }

    return 0;
}