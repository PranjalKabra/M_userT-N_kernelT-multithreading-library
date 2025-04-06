#include "../include/mn_thread.h"
#include <stdlib.h>
#include <stdio.h>

void mn_thread_yield(int current_thread_index) {
    int prev_thread = current_thread_index;
    int current_kthread = uthreads[prev_thread].kernel_thread_id;
    
    printf("Yielding from thread %d (K-Thread %d)\n", prev_thread, current_kthread);

    // Save current thread state if not terminated
    if (uthreads[prev_thread].state != THREAD_TERMINATED) {
        uthreads[prev_thread].state = THREAD_READY;
    }

    // Find the next thread in the same kernel thread group
    int next_thread = -1;
    int threads_checked = 0;
    int num_kthreads = 4;
    //int base_index = current_kthread;  // Start with the kernel thread ID
    
    // Calculate the next thread ID based on the thread allocation pattern
    // Pattern is: k_id + j*NUM_KTHREADS where j is 0 to THREADS_PER_KTHREAD-1
    // Find current j value for prev_thread
    int j_value = (prev_thread - current_kthread) / num_kthreads;
    
    // Try the next j value, or wrap around to 0
    j_value = (j_value + 1) % 4;
    next_thread = current_kthread + (j_value * 4);
    
    // If the calculated thread is terminated, find the next available one
    while (uthreads[next_thread].state == THREAD_TERMINATED && threads_checked < 4) {
        j_value = (j_value + 1) % 4;
        next_thread = current_kthread + (j_value * 4);
        threads_checked++;
    }
    
    // If all threads are terminated, return to kernel thread
    if (threads_checked >= 4) {
        printf("All threads in kernel thread %d terminated.\n", current_kthread);
        current_thread_per_kthread[current_kthread] = -1;
        
        // Return to the kernel thread's context
        if (swapcontext(&uthreads[prev_thread].context, 
                        &kthreads[current_kthread].k_context) == -1) {
            perror("Failed to swap context back to kernel thread");
        }
        return;
    }
    
    // Update the currently running thread for this kernel thread
    current_thread_per_kthread[current_kthread] = next_thread;
    
    printf("Switching to thread %d (K-Thread %d)\n", 
           next_thread, current_kthread);
    
    // Set next thread to RUNNING
    uthreads[next_thread].state = THREAD_RUNNING;

    // Switch context
    if (swapcontext(&uthreads[prev_thread].context, &uthreads[next_thread].context) == -1) {
        perror("Failed to swap context");
        return;
    }
}

void mn_thread_exit(int current_thread_index) {
    printf("Exiting thread %d (K-Thread %d)\n", 
           current_thread_index, uthreads[current_thread_index].kernel_thread_id);
    uthreads[current_thread_index].state = THREAD_TERMINATED;
    
    // Free stack memory safely
    if (uthreads[current_thread_index].context.uc_stack.ss_sp) {
        free(uthreads[current_thread_index].context.uc_stack.ss_sp);
        uthreads[current_thread_index].context.uc_stack.ss_sp = NULL;
    }
    
    mn_thread_yield(current_thread_index);
}

