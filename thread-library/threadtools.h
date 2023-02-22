#ifndef THREADTOOL
#define THREADTOOL

#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#define THREAD_MAX 16  // maximum number of threads created
#define BUF_SIZE 512
struct tcb
{
	int id;  // the thread id
	jmp_buf environment;  // where the scheduler should jump to
	int arg;  // argument to the function
	int fd;  // file descriptor for the thread
	char buf[BUF_SIZE];  // buffer for the thread
	char fifoName[512];
	int i, x, y;  // declare the variables you wish to keep between switches
};

extern int timeslice;
extern jmp_buf sched_buf;
extern struct tcb *ready_queue[THREAD_MAX], *waiting_queue[THREAD_MAX];
/*
 * rq_size: size of the ready queue
 * rq_current: current thread in the ready queue
 * wq_size: size of the waiting queue
 */
extern int rq_size, rq_current, wq_size;
/*
 * base_mask: blocks both SIGTSTP and SIGALRM
 * tstp_mask: blocks only SIGTSTP
 * alrm_mask: blocks only SIGALRM
 */
extern sigset_t base_mask, tstp_mask, alrm_mask;
/*
 * Use this to access the running thread.
 */
#define RUNNING (ready_queue[rq_current])

void sighandler(int signo);

void scheduler();

#define thread_create(func, id, arg) \
{                                    \
	func(id, arg);                   \
}

#define thread_setup(id, arg)                                       \
{                                                                   \
	struct tcb *thread =(struct tcb *) malloc(sizeof( struct tcb)); \
	thread->id = id;                                                \
	thread->arg = arg;                                              \
	sprintf(thread->fifoName, "%d_%s", id, __func__);               \
	mkfifo(thread->fifoName, 0666);                                 \
	thread->fd = open(thread->fifoName,  O_RDONLY | O_NONBLOCK);    \
	if (setjmp(thread->environment) == 0)                           \
	{                                                               \
		ready_queue[rq_size++] = thread;                            \
		return;                                                     \
	}                                                               \
}

#define thread_exit()          \
{                              \
	close(RUNNING->fd);        \
	unlink(RUNNING->fifoName); \
	longjmp(sched_buf, 3);     \
}

#define thread_yield()                              \
{                                                   \
	if (setjmp(RUNNING->environment) == 0)          \
	{                                               \
		sigprocmask(SIG_SETMASK, &alrm_mask, NULL); \
		sigprocmask(SIG_SETMASK, &tstp_mask, NULL); \
		sigprocmask(SIG_SETMASK, &base_mask, NULL); \
	}                                               \
}

#define async_read(count)                       \
{                                               \
	if (setjmp(RUNNING->environment) == 0)      \
	{                                           \
		longjmp(sched_buf, 2);                  \
	}                                           \
	else                                        \
	{                                           \
		read(RUNNING->fd, RUNNING->buf, count); \
	}\
}

#endif // THREADTOOL
