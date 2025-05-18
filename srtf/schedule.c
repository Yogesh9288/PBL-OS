#include "inc.h"
#include <minix/com.h>
#include <stdlib.h>
#include <assert.h>

/* Pick the process with the shortest remaining time */
static int pick_srtf_process(void);

/===========================================================================
 *                              do_schedule                                  *
 ===========================================================================/
PUBLIC int do_schedule(message *m_ptr)
{
    int proc_nr = m_ptr->SCHEDULING_ENDPOINT;

    if (proc_nr < 0 || proc_nr >= NR_PROCS) return EINVAL;

    /* Initialize priority if not set */
    if (schedproc[proc_nr].priority < 0)
        schedproc[proc_nr].priority = USER_Q;

    /* Initialize remaining time field if needed */
    if (schedproc[proc_nr].remaining_time == 0)
        schedproc[proc_nr].remaining_time = DEFAULT_BURST_TIME;  // You can define a constant

    struct schedparam sparam;
    sparam.sched_priority = schedproc[proc_nr].priority;

    return sched_setparam(proc_nr, &sparam);
}

/===========================================================================
 *                              do_noquantum                                 *
 ===========================================================================/
PUBLIC int do_noquantum(message *m_ptr)
{
    int next_proc = pick_srtf_process();

    if (next_proc >= 0) {
        struct schedparam sparam;
        sparam.sched_priority = schedproc[next_proc].priority;
        sched_setparam(next_proc, &sparam);
    }

    return OK;
}

/===========================================================================
 *                          pick_srtf_process                                *
 ===========================================================================/
static int pick_srtf_process(void)
{
    int i, chosen_proc = -1;
    int min_time = INT_MAX;

    for (i = 0; i < NR_PROCS; i++) {
        if (schedproc[i].in_use && schedproc[i].remaining_time < min_time) {
            min_time = schedproc[i].remaining_time;
            chosen_proc = i;
        }
    }

    return chosen_proc;
}
