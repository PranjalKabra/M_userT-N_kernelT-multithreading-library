#include "../include/mn_thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#define N 15 // Must be a multiple of 3

long long int partial_products[3]; // Stores results from each kernel thread
long long int thread_results[6]; // Store individual thread results

void compute_partial_factorial(void* arg) {
    int thread_id = *(int*)arg;
    int kernel_thread_id = uthreads[thread_id].kernel_thread_id;
    
    // Determine which half of the range this thread is responsible for
    int is_second_thread = (thread_id % 2 == 1);
    
    // Calculate the range for each kernel thread
    int chunk_size = N / num_kthreads;
    int start_k = kernel_thread_id * chunk_size + 1;
    int end_k = start_k + chunk_size - 1;
    
    // Divide the kernel thread's range between its two user threads
    int mid = (start_k + end_k) / 2;
    int start, end;
    
    if (!is_second_thread) {
        // First user thread takes first half
        start = start_k;
        end = mid;
    } else {
        // Second user thread takes second half
        start = mid + 1;
        end = end_k;
    }
    
    // Compute the partial product
    long long product = 1LL;
    for (int i = start; i <= end; i++) {
        product *= i;
    }
    // Store the result
    thread_results[thread_id] = product;
    
    printf("[K-Thread %d] U-Thread-%d computed product [%d-%d] = %lld\n", 
           kernel_thread_id, thread_id, start, end, product);
    
    
    
    // Mark thread as terminated
    uthreads[thread_id].state = THREAD_TERMINATED;
    
    // Yield to allow the other thread to run
    mn_thread_yield(thread_id);
}

void* kernel_thread_function(void* arg) {
    mn_kernel_thread_t* kthread = (mn_kernel_thread_t*)arg;
    int k_id = kthread->id;
    
    // Initialize the partial product for this kernel thread
    partial_products[k_id] = 1LL;
    
    // Get the context of the kernel thread
    getcontext(&kthread->k_context);

    if (getcontext(&kthread->k_context) == -1) {
        perror("getcontext failed");
        return NULL;
    }
    
    // Set the current thread for this kernel thread to the first assigned thread
    if (kthread->num_assigned > 0) {
        int first_thread_id = kthread->assigned_threads[0]->id;
        kthread->current_thread = kthread->assigned_threads[0];
        current_thread_per_kthread[k_id] = first_thread_id;
        
        printf("[K-Thread %d] Starting first U-Thread-%d\n", k_id, first_thread_id);
        uthreads[first_thread_id].state = THREAD_RUNNING;
        
        // Switch to the first thread
        if (swapcontext(&kthread->k_context, &uthreads[first_thread_id].context) == -1) {
            perror("swapcontext failed");
            return NULL;
        }
    }
    
    // Wait for both user threads to complete
    mn_thread_wait(kthread);
    
    // Compute the partial product from both user threads
    for (int i = 0; i < kthread->num_assigned; i++) {
        int thread_id = kthread->assigned_threads[i]->id;
        if (thread_results[thread_id] != 0) {  // Add check for valid result
            partial_products[k_id] *= thread_results[thread_id];
        }
    }
    
    printf("[K-Thread %d] Partial product = %lld\n", k_id, partial_products[k_id]);
    
    return NULL;
}

int main() {
    // Initialize with 6 user threads, 3 kernel threads, no preemption (0, 0)
    mn_thread_init(6, 3, 0, 0);
    int* thread_ids[num_uthreads];
    
    // Initialize thread arrays
    for (int i = 0; i < num_kthreads; i++) {
        kthreads[i].id = i;
        kthreads[i].num_assigned = 0;
        current_thread_per_kthread[i] = 0;
        partial_products[i] = 1LL;
    }
    
    printf("\n=== Factorial Calculation with M:N Threading ===\n");
    printf("Configuration: %d user threads mapped to %d kernel threads\n", 
           num_uthreads, num_kthreads);
    printf("Calculating %d! (factorial)\n\n", N);
    
    // Create and assign user threads to kernel threads
    for (int i = 0; i < num_uthreads; i++) {
        thread_ids[i] = malloc(sizeof(int));
        *thread_ids[i] = i;
        
        // Determine which kernel thread this user thread belongs to
        int kernel_thread_id = i % num_kthreads;
        
        // Initialize user thread
        uthreads[i].id = i;
        uthreads[i].kernel_thread_id = kernel_thread_id;
        uthreads[i].state = THREAD_READY;
        
        // Allocate stack for the user thread
        uthreads[i].context.uc_stack.ss_sp = malloc(STACK_SIZE);
        if (uthreads[i].context.uc_stack.ss_sp == NULL) {
            perror("Failed to allocate stack for user thread");
            return -1;
        }
        uthreads[i].context.uc_stack.ss_size = STACK_SIZE;
        uthreads[i].context.uc_stack.ss_flags = 0;
        uthreads[i].context.uc_link = &kthreads[kernel_thread_id].k_context;
        
        // Create the user thread
        if (mn_thread_create(&uthreads[i], compute_partial_factorial, thread_ids[i]) != 0) {
            printf("Thread creation failed\n");
            return -1;
        }
        
        // Assign the user thread to its kernel thread
        kthreads[kernel_thread_id].assigned_threads[kthreads[kernel_thread_id].num_assigned++] = &uthreads[i];
    }
    
    // Start kernel threads
    for (int i = 0; i < num_kthreads; i++) {
        pthread_create(&kthreads[i].pthread, NULL, kernel_thread_function, &kthreads[i]);
    }
    
    // Wait for kernel threads to complete
    for (int i = 0; i < num_kthreads; i++) {
        pthread_join(kthreads[i].pthread, NULL);
    }
    
    // Calculate final factorial
    long long int factorial = 1LL;
    for (int i = 0; i < num_kthreads; i++) {
        factorial *= partial_products[i];
    }
    
    printf("\nFinal Factorial (N=%d) = %lld\n", N, factorial);
    
    // Clean up
    for (int i = 0; i < num_uthreads; i++) {
        free(thread_ids[i]);
        free(uthreads[i].context.uc_stack.ss_sp);
    }
    
    return 0;
}