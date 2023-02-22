#include "threadtools.h"

/*
 * Print out the signal you received.
 * If SIGALRM is received, reset the alarm here.
 * This function should not return. Instead, call longjmp(sched_buf, 1).
 */
void sighandler(int signo)
{
    if (signo == SIGTSTP)
        printf("caught SIGTSTP\n");
    else if (signo == SIGALRM)
    {
        printf("caught SIGALRM\n");
        alarm(timeslice);
    }
    longjmp(sched_buf, 1);
}

/*
 * Prior to calling this function, both SIGTSTP and SIGALRM should be blocked.
 */
void scheduler()
{
    int ret = setjmp(sched_buf);

    if (ret == 0)
    {
        longjmp(ready_queue[0]->environment, 1);
    }
    else
    {
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        fd_set working_set, master_set;

        FD_ZERO(&working_set);
        for (int i = 0; i < wq_size; i++)
            FD_SET(waiting_queue[i]->fd, &working_set);
        if (select(1000, &working_set, NULL, NULL, &timeout) > 0)
        {
            for (int i = 0; i < wq_size; i++)
            {
                if (FD_ISSET(waiting_queue[i]->fd, &working_set))
                {
                    ready_queue[rq_size++] = waiting_queue[i];
                    waiting_queue[i] = NULL;
                }
            }
            int modify_idx = 0;
            for (int i = 0; i < wq_size; i++)
            {
                if (waiting_queue[i] == NULL)
                    continue;
                waiting_queue[modify_idx++] = waiting_queue[i];
            }
            wq_size = modify_idx;
        }

        if (ret == 2)
        {
            waiting_queue[wq_size++] = RUNNING;
            rq_size--;
        }
        else if (ret == 3)
        {
            free(RUNNING);
            rq_size--;
        }

        if (rq_size > 0)
        {
            if (ret == 1)
                rq_current = (rq_current + 1) % rq_size;
            else
            {
                RUNNING = ready_queue[rq_size];
                ready_queue[rq_size] = NULL;
                if (rq_current == rq_size)
                    rq_current = 0;
            }
        }
        else
        {
            if (wq_size > 0)
            {
                int flag = 1;
                FD_ZERO(&master_set);
                for (int i = 0; i < wq_size; i++)
                    FD_SET(waiting_queue[i]->fd, &master_set);
                while (flag)
                {
                    working_set = master_set;
                    if (select(1000, &working_set, NULL, NULL, &timeout) > 0)
                    {
                        flag = 0;
                        for (int i = 0; i < wq_size; i++)
                        {
                            if (FD_ISSET(waiting_queue[i]->fd, &working_set))
                            {
                                ready_queue[rq_size++] = waiting_queue[i];
                                waiting_queue[i] = NULL;
                            }
                        }
                        int modify_idx = 0;
                        for (int i = 0; i < wq_size; i++)
                        {
                            if (waiting_queue[i] == NULL)
                                continue;
                            waiting_queue[modify_idx++] = waiting_queue[i];
                        }
                        wq_size = modify_idx;
                        rq_current = 0;
                    }
                }
            }
            else
                return;
        }
        longjmp(RUNNING->environment, 1);
    }
}
