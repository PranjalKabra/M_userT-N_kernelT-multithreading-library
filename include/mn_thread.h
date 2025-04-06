#ifndef MN_THREAD_H
#define MN_THREAD_H

#include <ucontext.h>
#include <pthread.h>
#include <stdint.h>

#define THREAD_RUNNING 0
#define THREAD_READY 1
#define THREAD_BLOCKED = 2 // not required as such inmy test examples
#define THREAD_TERMINATED 3
#define MAX_KTHREADS 4    // Maximum kernel threads
#define MAX_UTHREADS 128   // Maximum user threads
#define STACK_SIZE  8192   // Stack size per thread -> 8 KB 

typedef struct mn_thread {
    ucontext_t context;
    int id;
    int kernel_thread_id;
    int state; // RUNNING, READY, TERMINATED
} mn_thread_t;

typedef struct mn_kernel_thread {
    pthread_t pthread;
    int id;
    mn_thread_t* assigned_threads[4]; // array of assigned user threads, for my example 4 is required
    int num_assigned; // number of user threadds assigned
    int current_index; // current thread whic is being executed
    ucontext_t k_context; // kernel thread context
    mn_thread_t* current_thread;
} mn_kernel_thread_t;

extern mn_thread_t uthreads[MAX_UTHREADS];
extern mn_kernel_thread_t kthreads[MAX_KTHREADS];
extern int current_thread_per_kthread[MAX_KTHREADS]; 
extern ucontext_t main_context; // main context for the main thread

// global variables -> to be intialized in example file while u are using in example file-> using mn_thread_init
extern int num_uthreads;
extern int num_kthreads;
extern int threads_per_kthread;
extern int burst_time;
extern int time_quantum;

// function prototypes
void mn_thread_init(int n_uthreads, int n_kthreads, int t_quantum, int b_time); // in mn_thread.c
int mn_thread_create(mn_thread_t* thread, void (*start_routine)(void*), void* arg); // in mn_thread.c
void mn_thread_yield(); // in scheduler.c
void mn_thread_exit(); // in scheduler.c
void mn_thread_wait(mn_kernel_thread_t* kthread); // in mn_thread.c
void mn_thread_map(); // in mn_thread.c


#endif

