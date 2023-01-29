#include "threadtools.h"

void fibonacci(int id, int arg)
{
    thread_setup(id, arg)
    for (RUNNING->i = 1;; RUNNING->i++)
    {
        if (RUNNING->i <= 2)
            RUNNING->x = RUNNING->y = 1;
        else
        {
            /* We don't need to save tmp, so it's safe to declare it here. */
            int tmp = RUNNING->y;
            RUNNING->y = RUNNING->x + RUNNING->y;
            RUNNING->x = tmp;
        }
        printf("%d %d\n", RUNNING->id, RUNNING->y);
        sleep(1);

        if (RUNNING->i == RUNNING->arg)
        {
            thread_exit()
        }
        else
        {
            thread_yield()
        }
    }
}

void collatz(int id, int arg)
{
    thread_setup(id, arg)
    for (RUNNING->i = 1;; RUNNING->i++)
    {
        if (RUNNING->arg % 2 == 0)
            RUNNING->arg /= 2;
        else
            RUNNING->arg = RUNNING->arg * 3 + 1;
        printf("%d %d\n", RUNNING->id, RUNNING->arg);
        sleep(1);

        if (RUNNING->arg == 1)
        {
            thread_exit()
        }
        else
        {
            thread_yield()
        }
    }
}

void max_subarray(int id, int arg)
{
    thread_setup(id, arg)
    for (RUNNING->i = 1;; RUNNING->i++)
    {
        async_read(5);
        // i = idx, x = max, y = previous
        int current = atoi(RUNNING->buf);
        if (RUNNING->i == 1)
        {
            RUNNING->y = current;
            RUNNING->x = (current > 0 ? current : 0);
        }
        else
        {
            if (RUNNING->y > 0)
                current += RUNNING->y;
            if (current > RUNNING->x)
                RUNNING->x = current;
            RUNNING->y = current;
        }
        printf("%d %d\n", RUNNING->id, RUNNING->x);
        sleep(1);

        if (RUNNING->i == RUNNING->arg)
        {
            thread_exit()
        }
        else
        {
            thread_yield()
        }
    }
}
