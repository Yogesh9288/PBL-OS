/* LRTF-based scheduling policy for SCHED */
#include "sched.h"
#include "schedproc.h"
#include <assert.h>
#include <minix/com.h>
#include <machine/archtypes.h>

#define DEFAULT_USER_TIME_SLICE 200

static int schedule_process(struct schedproc * rmp, unsigned flags);

#define SCHEDULE_CHANGE_PRIO    0x1
#define SCHEDULE_CHANGE_QUANTUM 0x2
#define SCHEDULE_CHANGE_CPU     0x4
#define SCHEDULE_CHANGE_ALL     (SCHEDULE_CHANGE_PRIO | SCHEDULE_CHANGE_QUANTUM | SCHEDULE_CHANGE_CPU)

#define schedule_process_local(p) schedule_process(p, SCHEDULE_CHANGE_PRIO | SCHEDULE_CHANGE_QUANTUM)
#define schedule_process_migrate(p) schedule_process(p, SCHEDULE_CHANGE_CPU)

#define CPU_DEAD -1
#define cpu_is_available(c) (cpu_proc[c] >= 0)
#define is_system_proc(p) ((p)->parent == RS_PROC_NR)

static unsigned cpu_proc[CONFIG_MAX_CPUS];

static void pick_cpu(struct schedproc * proc)
{
#ifdef CONFIG_SMP
    unsigned cpu, c;
    unsigned cpu_load = (unsigned) -1;

    if (machine.processors_count == 1) {
        proc->cpu = machine.bsp_id;
        return;
    }

    if (is_system_proc(proc)) {
        proc->cpu = machine.bsp_id;
        return;
    }

    cpu = machine.bsp_id;
    for (c = 0; c < machine.processors_count; c++) {
        if (!cpu_is_available(c))
            continue;
        if (c != machine.bsp_id && cpu_load > cpu_proc[c]) {
            cpu_load = cpu_proc[c];
            cpu = c;
        }
    }
    proc->cpu = cpu;
    cpu_proc[cpu]++;
#else
    proc->cpu = 0;
#endif
}

int do_noquantum(message *m_ptr)
{
    struct schedproc *rmp;
    int rv, proc_nr_n;

    if (sched_isokendpt(m_ptr->m_source, &proc_nr_n) != OK) {
        printf("SCHED: WARNING: got invalid endpoint in OOQ msg %u.\n",
               m_ptr->m_source);
        return EBADEPT;
    }

    rmp = &schedproc[proc_nr_n];

    // Subtract used time slice
    rmp->remaining_time -= rmp->time_slice;

    // Reschedule the process with the longest remaining time
    struct schedproc *selected = NULL;
    for (int i = 0; i < NR_PROCS; ++i) {
        if (schedproc[i].flags & IN_USE) {
            if (!selected || schedproc[i].remaining_time > selected->remaining_time) {
                selected = &schedproc[i];
            }
        }
    }

    if (selected && (rv = schedule_process_local(selected)) != OK) {
        return rv;
    }

    return OK;
}

int do_stop_scheduling(message *m_ptr)
{
    struct schedproc *rmp;
    int proc_nr_n;

    if (!accept_message(m_ptr))
        return EPERM;

    if (sched_isokendpt(m_ptr->m_lsys_sched_scheduling_stop.endpoint,
                        &proc_nr_n) != OK) {
        printf("SCHED: WARNING: got invalid endpoint in stop msg %d\n",
               m_ptr->m_lsys_sched_scheduling_stop.endpoint);
        return EBADEPT;
    }

    rmp = &schedproc[proc_nr_n];
#ifdef CONFIG_SMP
    cpu_proc[rmp->cpu]--;
#endif
    rmp->flags = 0;
    return OK;
}

int do_start_scheduling(message *m_ptr)
{
    struct schedproc *rmp;
    int rv, proc_nr_n;

    assert(m_ptr->m_type == SCHEDULING_START || m_ptr->m_type == SCHEDULING_INHERIT);

    if (!accept_message(m_ptr))
        return EPERM;

    if ((rv = sched_isemtyendpt(m_ptr->m_lsys_sched_scheduling_start.endpoint,
                                &proc_nr_n)) != OK) {
        return rv;
    }

    rmp = &schedproc[proc_nr_n];

    rmp->endpoint = m_ptr->m_lsys_sched_scheduling_start.endpoint;
    rmp->parent = m_ptr->m_lsys_sched_scheduling_start.parent;

    rmp->priority = USER_Q;
    rmp->max_priority = USER_Q;
    rmp->time_slice = DEFAULT_USER_TIME_SLICE;

    // Initialize remaining time to some default (e.g., 1000 ms)
    rmp->remaining_time = 1000;

#ifdef CONFIG_SMP
    rmp->cpu = machine.bsp_id;
#endif

    if ((rv = sys_schedctl(0, rmp->endpoint, 0, 0, 0)) != OK) {
        printf("Sched: Error taking over scheduling for %d, kernel said %d\n",
               rmp->endpoint, rv);
        return rv;
    }

    rmp->flags = IN_USE;
    pick_cpu(rmp);

    while ((rv = schedule_process(rmp, SCHEDULE_CHANGE_ALL)) == EBADCPU) {
        cpu_proc[rmp->cpu] = CPU_DEAD;
        pick_cpu(rmp);
    }

    if (rv != OK) {
        printf("Sched: Error while scheduling process, kernel replied %d\n", rv);
        return rv;
    }

    m_ptr->m_sched_lsys_scheduling_start.scheduler = SCHED_PROC_NR;
    return OK;
}

int do_nice(message *m_ptr)
{
    return OK;
}

static int schedule_process(struct schedproc * rmp, unsigned flags)
{
    int err;
    int new_prio = (flags & SCHEDULE_CHANGE_PRIO) ? rmp->priority : -1;
    int new_quantum = (flags & SCHEDULE_CHANGE_QUANTUM) ? rmp->time_slice : -1;
    int new_cpu = (flags & SCHEDULE_CHANGE_CPU) ? rmp->cpu : -1;
    int niced = 0;

    if ((err = sys_schedule(rmp->endpoint, new_prio, new_quantum, new_cpu, niced)) != OK) {
        printf("PM: Error scheduling %d: %d\n", rmp->endpoint, err);
    }

    return err;
}

void init_scheduling(void)
{
    // No periodic rebalancing needed
}

void balance_queues(void)
{
    // No-op for LRTF
}
