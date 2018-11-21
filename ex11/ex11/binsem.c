//
// Created by avichay ben lulu 301088670.
// desc.:an implementation of a simple binary semaphores library for user level threads.
#include "binsem.h"
#include <signal.h>
#include <unistd.h>

//Initializes a binary semaphore.
void binsem_init(sem_t *s, int init)
{
    *s = init;
}

void binsem_up(sem_t *s)
{
    xchg(s,1);
}

int binsem_down(sem_t *s)
{
    while (xchg(s,0) == 0)
        if (kill(getpid(),SIGALRM)== -1) return -1;

    return 0;
}
