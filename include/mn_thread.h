#ifndef MN_THREAD_H
#define MN_THREAD_H

#include <ucontext.h>
#include <pthread.h>
#include <stdint.h>

// not really required here
#define THREAD_RUNNING 0
#define THREAD_READY 1
#define THREAD_BLOCKED = 2
#define THREAD_TERMINATED 3
#define MAX_KTHREADS 4    // Maximum kernel threads
#define MAX_UTHREADS 128   // Maximum user threads
#define STACK_SIZE  8192   // Stack size per thread

// roundrobin declarations
#define TIME_QUANTUM 5
#define BURST_TIME 12
#define THREADS_PER_KTHREAD 4

typedef struct mn_thread {
    ucontext_t context;
    int id;
    int kernel_thread_id;
    int state; // RUNNING, READY, TERMINATED
   // struct mn_thread* next;
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


//extern int next_tid;
//extern ucontext_t main_context;
// next_tid is a global variable that keeps track of the next thread ID to be assigned.
// API Functions
int mn_thread_create(mn_thread_t* thread, void (*start_routine)(void*), void* arg);
void mn_thread_yield();
void mn_thread_exit();
void mn_thread_cancel(mn_thread_t* thread);
void mn_thread_join(mn_thread_t* thread);
void mn_thread_map();


#endif

