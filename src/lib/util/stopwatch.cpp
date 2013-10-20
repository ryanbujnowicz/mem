#include "util/stopwatch.h"
using namespace util;

Stopwatch::Stopwatch() :
    _running(false)
{
    reset();
}

Stopwatch::~Stopwatch()
{
    // ILB
}

void Stopwatch::start()
{
    _running = true;
}

void Stopwatch::stop()
{
#ifdef HAS_SYS_TIME
    gettimeofday(&_end, 0);
#endif

    _running = false;
}

void Stopwatch::reset()
{
#ifdef HAS_SYS_TIME
    gettimeofday(&_start, 0);
#endif
}

double Stopwatch::getElapsed()
{
#ifdef HAS_SYS_TIME
    if (_running) {
        gettimeofday(&_end, 0);
    }
    return (_end.tv_sec - _start.tv_sec) + (_end.tv_usec - _start.tv_usec) / 1000000.0;
#else
#error Not implemented
#endif
}
