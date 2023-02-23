# User-Level Thread Library

## Tech Stack

- signal handling (sigprocmask, sigemptyset, sigaddset)
- alarm
- setjmp / longjmp
- select multiplexing

# Description

In this project, I'm going to implement a toy example of a user-level thread library.
Additionally, I will simulate asynchronous file I/O.

More specifically, there are going to be multiple user-defined threads running simultaneously.
Each thread is in charge of solving a computational problem (Fibonacci number, Collatz’s conjecture, and Maximum subarray).
To the kernel, there is only one process running, so there will never be two threads running at the same time.
Instead, I use setjmp and longjmp to context switch between these threads,
and make sure they each have a fair share of execution time.
When a thread needs to read from an external file, it should be put to sleep and let other threads execute.
Otherwise, the whole process would have to pause until the call to read returns.

We define three possible states for each thread:

- **Running.** The thread is running and occupying the CPU resource.
- **Ready.** The thread has all the input it needs at the moment, and it’s waiting for the CPU resource.
- **Waiting.** The thread is waiting for specific file input.

And we maintain two queues: a **ready queue** and a **waiting queue**.
The ready queue stores the **running** and **ready** threads,
while the waiting queue stores the **waiting** threads.
We will then write a scheduler to manage those queues.

## Overview

Here is an overview of how each file in the repository work. More detailed explanations are in the next section.

- `main.c` initializes the data structures needed by the thread library, creates the threads, then hands over to `scheduler.c`
- `scheduler.c` consists of a signal handler and a scheduler.
    - In this assignment, we repurpose two signals to help us perform context-switching.
      SIGTSTP can be triggered by pressing Ctrl+Z on your keyboard, and SIGALRM is triggered by the `alarm` syscall.
    - The scheduler maintains the waiting queue and the ready queue.
      Each time the scheduler is triggered, it iterates through the waiting queue to check if any of the requested data are ready.
      Afterward, it brings all available threads into the ready queue.
    - There are two reasons for a thread to leave the ready queue:
        - The thread has finished executing.
        - The thread wants to read from a file, whether or not the data is ready. In this case, this thread is moved to the waiting queue.
- `threads.c` defines the threads that are going to run in parallel. The lifecycle of a thread should be:
    - Created by calling `thread_create` in `main.c`.
    - Calls `thread_setup` to initialize itself.
    - When a small portion of computation is done, call `thread_yield` to check if a context switch is needed.
    - When it needs to read from a file, call `async_read` to gather the data.
    - After all computations are done, call `thread_exit` to clean up.

## Reference

[SPEC Link](https://hackmd.io/@aoaaceai/rka6PBfHj)