#ifndef UTIL_UNITS_H 
#define UTIL_UNITS_H 

namespace util {

constexpr inline double seconds(double s) 
{
    return s;
}

constexpr inline double milliseconds(double ms) 
{
    return ms/1000.0;
}

constexpr inline double minutes(double m) 
{
    return m*60.0;
}

constexpr inline double hours(double h) 
{
    return h*3600.0;
}

constexpr inline double days(double d) 
{
    return d*86400.0;
}

constexpr inline double weeks(double w) 
{
    return w*604800.0;
}

constexpr inline double meters(double m) 
{
    return m/100.0;
}

constexpr inline double centimeters(double cm) 
{
    return cm;
}

constexpr inline double millimeters(double mm) 
{
    return mm*10.0;
}

constexpr inline double kilometers(double km) 
{
    return km*100000.0;
}

constexpr inline size_t bytes(size_t b) 
{
    return b;
}

constexpr inline size_t kilobytes(size_t kb) 
{
    return kb*1024;
}

constexpr inline size_t megabytes(size_t mb) 
{
    return mb*1024*1024;
}

constexpr inline size_t gigabytes(size_t gb) 
{
    return gb*1024*1024*1024;
}

}

#endif

