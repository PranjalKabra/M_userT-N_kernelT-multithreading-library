#include "../include/mn_thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

mn_thread_t uthreads[MAX_UTHREADS];
mn_kernel_thread_t kthreads[MAX_KTHREADS];
ucontext_t main_context;
int current_thread_per_kthread[MAX_KTHREADS];

// initialize the global variables
int num_uthreads;
int num_kthreads;
int threads_per_kthread;
int burst_time;
int time_quantum;

void mn_thread_init( int n_uthreads,  int n_kthreads,  int t_quantum,  int b_time) {
    num_uthreads = n_uthreads;
    num_kthreads = n_kthreads;
    threads_per_kthread = n_uthreads / n_kthreads;
    time_quantum = t_quantum;
    burst_time = b_time;
}

void wrapper_function(void* func, void* arg) {
    void (*start_routine)(void*) = func;
    start_routine(arg);
    mn_thread_exit();
}


int mn_thread_create(mn_thread_t* thread, void (*start_routine)(void*), void* arg) {
    if (thread->id >= MAX_UTHREADS) return -1;

    thread->state = 1; // readt_state
    thread->kernel_thread_id = thread->kernel_thread_id;   /// alloted pehle se before creating the user thread

    if (getcontext(&thread->context) == -1) {
        perror("Failed to get context");
        return -1;
    }
    thread->context.uc_stack.ss_sp = malloc(STACK_SIZE);
    if (thread->context.uc_stack.ss_sp == NULL) {
        perror("Failed to allocate stack");
        return -1;
    }
    thread->context.uc_stack.ss_size = STACK_SIZE;
    thread->context.uc_stack.ss_flags = 0;
    thread->context.uc_link = NULL;

    makecontext(&thread->context, (void(*)())wrapper_function, 2, start_routine, arg);

    uthreads[thread->id] = *thread;
    printf("Created thread %d\n", thread->id);

    return 0;
}

void mn_thread_wait(mn_kernel_thread_t* kthread) {
    while (1) {
        int completed = 0;

        // check if all user threads assigned to this kernel thread have terminated
        for (int i = 0; i < threads_per_kthread; i++) {
            if (kthread->assigned_threads[i]->state == THREAD_TERMINATED) {
                completed++;
            }
        }

        // if both user threads have completed, exit the wait loop
        if (completed == threads_per_kthread) {
            return;
        }

        // sleep briefly to avoid busy waiting
        sleep(1); 
    }
}


void mn_thread_map() {
    printf("User Thread -> Kernel Thread Mapping:\n");
    for (int i = 0; i < num_uthreads; i++) {
        printf("User Thread %d -> Kernel Thread %d\n", uthreads[i].id, uthreads[i].kernel_thread_id);
    }
}