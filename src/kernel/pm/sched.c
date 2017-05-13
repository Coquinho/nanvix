/*
 * Copyright(C) 2011-2016 Pedro H. Penna   <pedrohenriquepenna@gmail.com>
 *              2015-2016 Davidson Francis <davidsondfgl@hotmail.com>
 *
 * This file is part of Nanvix.
 *
 * Nanvix is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nanvix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nanvix. If not, see <http://www.gnu.org/licenses/>.
 */

#include <nanvix/clock.h>
#include <nanvix/const.h>
#include <nanvix/hal.h>
#include <nanvix/pm.h>
#include <signal.h>

#include "../../lib/libc/stdlib/rand.c"
#include "../../lib/libc/stdlib/srand.c"
#include <nanvix/klib.h>

/**
 * @brief Schedules a process to execution.
 *
 * @param proc Process to be scheduled.
 */
PUBLIC void sched(struct process *proc)
{
    proc->state = PROC_READY;
    proc->counter = 0;
}

/**
 * @brief Stops the current running process.
 */
PUBLIC void stop(void)
{
    curr_proc->state = PROC_STOPPED;
    sndsig(curr_proc->father, SIGCHLD);
    yield();
}

/**
 * @brief Resumes a process.
 *
 * @param proc Process to be resumed.
 *
 * @note The process must stopped to be resumed.
 */
PUBLIC void resume(struct process *proc)
{
    /* Resume only if process has stopped. */
    if (proc->state == PROC_STOPPED)
        sched(proc);
}

/**
 * @brief Yields the processor.
 */
PUBLIC void yield(void)
{
    struct process *next; /* Next process to run. */

    /* Number of tickets until now. */
    int ntickets = 0;

    srand(123123);

    /* Re-schedule process for execution. */
    if (curr_proc->state == PROC_RUNNING)
        sched(curr_proc);

    /* Remember this process. */
    last_proc = curr_proc;

    /* Check alarm.  */
    for (next = FIRST_PROC; next <= LAST_PROC; next++)
    {
        /* Skip invalid processes. */
        if (!IS_VALID(next))
            continue;

        /* Alarm has expired. */
        if ((next->alarm) && (next->alarm < ticks))
            next->alarm = 0, sndsig(next, SIGALRM);
    }

    /* Give and count total number of tickets */
    for (next = FIRST_PROC; next <= LAST_PROC; next++)
        /* Give tickets to ready process. */
        if (next->state == PROC_READY) {
            /* Number of tickets based on priority an nice level. */
            next->counter = (2^(next->priority < 0 ? (-1)*(next->priority) + 40 : next->priority))-(2^next->nice);
            /* Count total number of tickets */
            ntickets += next->counter;
        }

    /* Sort a ticket.*/
    int ticket = rand() % ntickets;

    /* Choose a process to run next. */

    int ticket_counter = 0;

    for (next = IDLE; next <= LAST_PROC; next++)
    {
        /* Skip non-ready process. */
        if (next->state != PROC_READY)
            continue;

        /* limit of tickets to evaluated now */
        ticket_counter += next->counter;

        /* Select the process that has The ticket. */
        if(ticket < ticket_counter)
            break;

    }

    /* Switch to next process. */
    next->priority = PRIO_USER;
    next->state = PROC_RUNNING;
    next->counter = PROC_QUANTUM;
    switch_to(next);
}
