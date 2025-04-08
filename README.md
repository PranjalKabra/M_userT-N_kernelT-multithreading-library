# M : N User - to - Kernel Level Multi Threading Library

## What is a thread?
A thread is the smallest unit of execution within a process.

## what is Multithreading ?
Multithreading in Operating System refers to the ability of a process to execute multiple threads concurrently withing the same process.

## Types of Threads:
**1. User Level Thread -**
- MAnaged by user level libraries, without kernel involvement.
- Have Faster Context Switches
- Block an entire process if one thread makes a blocking system call

**2. Kernel Level Thread -**
- Managed directly by OS
- Allows true parallelism

## Different multithreading models 
- Many to one Model : Maps many user threads to a single kernel thread. Simple but no Parallel execution
- One to one model : Maps each user thread to a kernel thread, better concurrency but increased overhead (due to context switch time amonst kernel threads)
- Many - to Many model : Maps many user threads to an equal or smaller amount of kernel thread. It is also known as M : N user to kernel level multithreading library, which is what this project about


## What is it?
This library demonstrates the M:N threading model, providing efficient thread management while maintaining good performance characteristics.

## Motivation ?
An M:N user-to-kernel multithreading library is efficient as it maps multiple user threads to fewer kernel threads, **reducing context-switching overhead**. It enables **custom scheduling**, **better resource utilization**, and improved concurrency for blocking operations.

## Features
1. Initialize the custom number of Kernel and User Threads
2. User and Kernel Thread Creation
3. Thread yielding : Premption amongst User Threads belonging to same Kernel Thread.
4. Thread Waiting : Waiting by a userthread for all its peer uthreads to terminate
5. Depiction  of Kernel to User thread Mapping
6. Example 1 : Http_simulation
7. Example 2 : Generation of n!


## Library structure
```
mn_thread_lib/
├── include/
│   └── mn_thread.h    # Header file with thread definitions
├── build/            # for compiled binaries and object files
├── src/
│   ├── mn_thread.c    # Core threading implementation
│   └── scheduler.c    # Thread scheduling implementation
├── test/
│   ├── test_http_sim.c  # HTTP request simulation test
│   └── test_fact.c      # Factorial calculation test
└── Makefile
```

## How to Build and Run ?

### Prerequisites
- Linux operating system
- GCC compiler
- Make utility

### Building the Project
1. Clone the repository:
```bash
git clone https://github.com/PranjalKabra/M_userT_Nkernel ...
cd mn_thread_lib
```

2. Build all tests:
```bash
make
```

### Running Individual Tests

1. HTTP Simulation Test:
```bash
make http_sim
```
TO 
- Compile the HTTP simulation test
- Create executable in build directory
- Run the simulation with 16 user threads mapped to 4 kernel threads

2. Factorial Calculation Test:
```bash
make test_fact
```
TO
- Compile the factorial calculation test
- Create executable in build directory
- Calculate 15! using 6 user threads mapped to 3 kernel threads
- Change N if you feel like , You can try with N = 18, by changing where N is defined in test_fact.c

### Cleaning Build Files
To remove all compiled files and clean the build directory:
```bash
make clean
```

### Build Directory Structure
After successful compilation, the `build/` directory will contain:
```
build/
├── test_http_sim    # HTTP simulation executable
└── test_fact        # Factorial calculation executable
```

## Test HTTP Simulation - Explaining the Output
So, The test file "test_http_sim.c" is a stimulation of how http requests when made are entertained by the server in the hypothesis that there is a main top server under whom are 4 servers, those 4 servers can work parallely, explaining the resemblance to **kernel threads**. 
Now I want to fetch information from 16 pages , resembling to **16 user threads**. Since at a time only 4 requests can be entertained(4 servers), 4 user threads are mapped to 4 kernel threads. But that would leave us to a **delayed response time for the other user threads**, so to improve responsiveness, **each kernel thread is alloted 4 user threads** and the user threads get their fair share of turn to request the service from server(here CPU)
Now the user threads are divided to kernel thread on the logiv that 
KERNEL THREAD0 -> U0,U4,U8,U12.
KERNEL THREAD1 -> U1,U5,U9,U13 .... and so on. 
Round Robin algorithm is implemented within each kernel thread and all user threads are prempted after a **time quantum of 5 units**. The **burst time of each user thread is declared as 12 units**. So premptions would occur when remaining burst time would be **12 - 5 = 7 units, then further ar 7 - 5 = 2 units** and since now remaning time < time quantum, the the thread will exit marking it's completion and no premption would be there.

## Test Factorial Calculation - Explaining the Output



