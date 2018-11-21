/*
date: 21/11/2018
author: avichay ben lulu - 301088670
desc.: an implementetion of a simple library to creating & scheduling user level threads
*/


#include "ut.h"
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>



static ut_slot thread_table;
static int tablesize;
static ucontext_t curr_context; // temp context

static tid_t curr_thread_num; // current thread number - for swapcontext
static tid_t prev_thread_num; // previous thread number - for swapcontext
static tid_t tid = 0; 

 
/*****************************************************************************
signal handler - handle the swaps between threads.as described at demo2
*****************************************************************************/

void handler(int signal)
{
    if (signal == SIGVTALRM)
		//update Vtime stats.
        thread_table[curr_thread_num].vtime+=100;

    if (signal == SIGALRM)
    {
		//perform swapcontext each sec
        alarm(1);
        prev_thread_num = curr_thread_num;
        curr_thread_num = (curr_thread_num + 1)% tablesize;
        if (swapcontext(&thread_table[prev_thread_num].uc,&thread_table[curr_thread_num].uc) == -1)
        {
            perror("swapcontext failed");
            exit(1);
        }
    }
}
/*****************************************************************************
 Initialize the library data structures. Create the threads table. 
 Parameters:
    tab_size - the threads_table_size.

 Returns:
    0 - on success.
	SYS_ERR - on table allocation failure.
*****************************************************************************/
int ut_init(int tab_size)
{
	//If the given size is otside the range [MIN_TAB_SIZE,MAX_TAB_SIZE], the table size will be MAX_TAB_SIZE.
    if(tab_size < MIN_TAB_SIZE || tab_size > MAX_TAB_SIZE)
        tab_size = MAX_TAB_SIZE;

    tablesize = tab_size;
    thread_table = (ut_slot)malloc((tab_size) * sizeof(ut_slot_t));

    if(thread_table == NULL) return SYS_ERR;

    return 0;
}


/*****************************************************************************
 Add a new thread to the threads table. 

 Parameters:
    func - a function to run in the new thread. We assume that the function is
	infinite and gets a single int argument.
	arg - the argument for func.

 Returns:
	non-negative TID of the new thread - on success (the TID is the thread's slot
	                                     number.
    SYS_ERR - on system failure (like failure to allocate the stack).
    TAB_FULL - if the threads table is already full.
 ****************************************************************************/
tid_t ut_spawn_thread(void (*func)(int), int arg)
{
    ut_slot  currSlot;
    int stackSize = STACKSIZE;
    if(tid >= tablesize) return TAB_FULL;

    currSlot = &thread_table[tid];

    currSlot->arg = arg;
    currSlot->func =func;
    currSlot->vtime = 0;

    if(getcontext(&currSlot->uc) == -1) return SYS_ERR;

    currSlot->uc.uc_link = &curr_context;
	// Allocate the thread stack and update the thread context accordingly.
    currSlot->uc.uc_stack.ss_sp = (void *)malloc(stackSize);
    if(currSlot->uc.uc_stack.ss_sp == NULL) return SYS_ERR; //if malloc failed call SYS_ERR

    currSlot->uc.uc_stack.ss_size = stackSize;
    makecontext(&currSlot->uc, (void(*)(void)) func, 1, arg);
    tid++;
    return tid-1;
}



/*****************************************************************************
 Starts running the threads, previously created by ut_spawn_thread. Sets the
 scheduler to switch between threads every second.
 Also starts the timer used to collect the threads CPU usage
 statistics and establishes an appropriate handler for SIGVTALRM,issued by the
 timer.
 

 Parameters:
    None.

 Returns:
    SYS_ERR - on system failure.
    Under normal operation, this function should start executing threads and
	never return.
 ****************************************************************************/
int ut_start(void)
{
    struct sigaction sa;
    struct itimerval itv;

    // The first thread to run is the thread with TID 0.
    itv.it_interval.tv_sec = 0;
    itv.it_interval.tv_usec = 100000;
    itv.it_value = itv.it_interval;
    //
    /* Initialize the data structures for SIGALRM handling. */
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);
    sa.sa_handler = handler;

    if (sigaction(SIGALRM, &sa, NULL) < 0) return SYS_ERR;
    
    if (sigaction(SIGVTALRM, &sa, NULL) < 0)
    return SYS_ERR;

    if (setitimer(ITIMER_VIRTUAL, &itv, NULL) < 0)
        return SYS_ERR;

    alarm(1);
    curr_thread_num = 0; // program starts with thread 0
    if (swapcontext(&curr_context, &thread_table[curr_thread_num].uc) == -1)
        return SYS_ERR;

    return 0;
}

/*****************************************************************************
 Returns the CPU-time consumed by the given thread.
 ****************************************************************************/
unsigned long ut_get_vtime(tid_t tid)
{
    return thread_table[tid].vtime;
}
