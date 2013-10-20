#ifndef UTIL_STOPWATCH_H
#define UTIL_STOPWATCH_H

#include "util/platform.h"

#ifdef HAS_SYS_TIME
#include <sys/time.h>
#else
#error Not implemented
#endif

namespace util {

/**
 * Measures elapsed time between start/end time samples.
 *
 * All results are returned as real valued numbers representing seconds.
 */
class Stopwatch
{
public:
    Stopwatch();
    ~Stopwatch();

    void start();
    void stop();
    void reset();
    double getElapsed();

private:
#ifdef HAS_SYS_TIME
    timeval _start;
    timeval _end;
#endif

    bool _running;
};

}

#endif

