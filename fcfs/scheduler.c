#include "inc.h"
#include <minix/com.h>
#include <stdlib.h>
#include <assert.h>

/* Pick the first-come-first-served process */
static int pick_fcfs_process(void);

/===========================================================================
 *                              do_schedule                                  *
 ===========================================================================/
PUBLIC int do_schedule(message *m_ptr)
{
    int proc_nr;

    proc_nr = m_ptr->SCHEDULING_ENDPOINT;
    if (proc_nr < 0 || proc_nr >= NR_PROCS) return EINVAL;

    if (schedproc[proc_nr].priority < 0)
        schedproc[proc_nr].priority = USER_Q;

    /* Set arrival time if it's the first time */
    if (schedproc[proc_nr].arrival_time == 0)
        schedproc[proc_nr].arrival_time = getticks();  // Needs arrival_time field

    struct schedparam sparam;
    sparam.sched_priority = schedproc[proc_nr].priority;
    return sched_setparam(proc_nr, &sparam);
}

/===========================================================================
 *                              do_noquantum                                 *
 ===========================================================================/
PUBLIC int do_noquantum(message *m_ptr)
{
    int next_proc = pick_fcfs_process();
    if (next_proc >= 0) {
        struct schedparam sparam;
        sparam.sched_priority = schedproc[next_proc].priority;
        sched_setparam(next_proc, &sparam);
    }
    return OK;
}

/===========================================================================
 *                          pick_fcfs_process                                *
 ===========================================================================/
static int pick_fcfs_process(void)
{
    int i;
    int earliest_proc = -1;
    clock_t earliest_time = LONG_MAX;

    for (i = 0; i < NR_PROCS; ++i) {
        if (schedproc[i].in_use && schedproc[i].arrival_time < earliest_time) {
            earliest_time = schedproc[i].arrival_time;
            earliest_proc = i;
        }
    }

    return earliest_proc;
}
