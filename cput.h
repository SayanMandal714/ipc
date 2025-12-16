
#ifndef CPUT_H
#define CPUT_H

#include <time.h>
#include <stdint.h>

/*
 * Thread CPU time utilities
 * Measures actual CPU time consumed by the calling thread
 * (does NOT include sleep / wait / blocking time)
 */

struct ThreadCpuTime {
    timespec ts;
};

/* Take CPU snapshot of current thread */
inline ThreadCpuTime threadCpuNow() {
    ThreadCpuTime t{};
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t.ts);
    return t;
}

/* Convert difference between two snapshots to milliseconds */
inline double threadCpuMs(const ThreadCpuTime& start,
                           const ThreadCpuTime& end)
{
    long sec  = end.ts.tv_sec  - start.ts.tv_sec;
    long nsec = end.ts.tv_nsec - start.ts.tv_nsec;

    return sec * 1000.0 + nsec / 1e6;
}

#endif // CPUT_H

