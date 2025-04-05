#include "../include/mn_thread.h"
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 7

void print_hello(void* arg) {
    //printf("Debug: arg = %p, *(int*)arg = %d\n", arg, *(int*)arg);
    int thread_id = *(int*)arg;
    int ker_th_id = uthreads[thread_id].kernel_thread_id;
    printf("Hello from thread %d, working on kernel thread %d\n", thread_id,ker_th_id);
    mn_thread_yield();
}

int main() {
    mn_thread_t threads[NUM_THREADS];
    int* thread_ids[NUM_THREADS];

    // initialize threead ids for 7 threads
    for(int i=0; i<NUM_THREADS; i++){
        thread_ids[i]  = malloc(sizeof(int));
        if(thread_ids[i] == NULL){
            perror("failed to allocate memory for some id\n");
            return -1;
        }
        *thread_ids[i] = i;

    }

    // crete num_threads threads
    for(int i=0; i<NUM_THREADS; i++){
        if(mn_thread_create(&threads[i],print_hello, thread_ids[i])!= 0){
            printf("ferror occurred while creating thread\n");
            return -1;
        }
    }

    mn_thread_map();

    // start scheduling from thread0
    int current_thread = 0;
    uthreads[current_thread].state = 0;  //curr thread is runnning
    swapcontext(&main_context, &uthreads[0].context); // switch to the thread

    int all_done;
    do{
        all_done = 1;
        for(int i=0; i<NUM_THREADS; i++){
            if(uthreads[i].state!=2){
                all_done =0;
                mn_thread_yield();
                break;
            }
        }
    }while(!all_done);

    // clean 
    for(int i=0; i<NUM_THREADS; i++){
        free(thread_ids[i]);
    }
    
    
    return 0;
}
