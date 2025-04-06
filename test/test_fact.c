#include "../include/mn_thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N 15 // Must be a multiple of 3

// M : USER THREADS -> 6
// N : KERNEL THREADS -> 3

long long int partial_products[3]; // to store the product carried out by each thread
long long int thread_results[6]; // to store indivodual result for each thread

void compute_partial_factorial(void* arg) {
    int thread_id = *(int*)arg;
    int kernel_thread_id = uthreads[thread_id].kernel_thread_id;
    
    // what half range will this thread get
    int is_second_thread = (thread_id % 2 == 1);
    
    // range for each k thread
    int chunk_size = N / num_kthreads;
    int start_k = kernel_thread_id * chunk_size + 1;
    int end_k = start_k + chunk_size - 1;
    
    // allot the kthread range to each uthread
    int mid = (start_k + end_k) / 2;
    int start, end;
    
    if (!is_second_thread) {
        start = start_k;
        end = mid;
    } else {
        start = mid + 1;
        end = end_k;
    }
    
    long long product = 1LL;
    for (int i = start; i <= end; i++) {
        product *= i;
    }
    // store the result
    thread_results[thread_id] = product;
    
    printf("[K-Thread %d] U-Thread-%d computed product [%d-%d] = %lld\n", kernel_thread_id, thread_id, start, end, product);
    
    uthreads[thread_id].state = THREAD_TERMINATED;
    
    // yield to allow toher thread in the same kernel group to run
    mn_thread_yield(thread_id);
}

void* kernel_thread_function(void* arg) {
    mn_kernel_thread_t* kthread = (mn_kernel_thread_t*)arg;
    int k_id = kthread->id;
   
    partial_products[k_id] = 1LL;
    
    // get the main context to be swithced to first user thread 
    getcontext(&kthread->k_context);

    if (getcontext(&kthread->k_context) == -1) {
        perror("getcontext failed");
        return NULL;
    }
    
    // if the code runs correctly, it'll always be first thread in it's kernel grp
    if (kthread->num_assigned > 0) {
        int first_thread_id = kthread->assigned_threads[0]->id;
        kthread->current_thread = kthread->assigned_threads[0];
        current_thread_per_kthread[k_id] = first_thread_id;
        
        printf("[K-Thread %d] Starting first U-Thread-%d\n", k_id, first_thread_id);
        uthreads[first_thread_id].state = THREAD_RUNNING;
        
        // allot cpu core to uthread by swapping context
        if (swapcontext(&kthread->k_context, &uthreads[first_thread_id].context) == -1) {
            perror("swapcontext failed");
            return NULL;
        }
    }
    
    // wait for both the user threads to complete
    mn_thread_wait(kthread);
    
    // after both threads has completed, u can find the product for the entire k thread
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
    // 6-> user thread, 3-> kernel thread, not using round robin -> so 0 : timequantum and burst time
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
        // Create the user thread
        if (mn_thread_create(&uthreads[i], compute_partial_factorial, thread_ids[i]) != 0) {
            printf("Thread creation failed\n");
            return -1;
        }
        
        // Assign the user thread to its kernel thread
        kthreads[kernel_thread_id].assigned_threads[kthreads[kernel_thread_id].num_assigned++] = &uthreads[i];
    }
    
    // start kernel thread
    for (int i = 0; i < num_kthreads; i++) {
        pthread_create(&kthreads[i].pthread, NULL, kernel_thread_function, &kthreads[i]);
    }
    
    //waiting for all kernel thread to join
    for (int i = 0; i < num_kthreads; i++) {
        pthread_join(kthreads[i].pthread, NULL);
    }

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