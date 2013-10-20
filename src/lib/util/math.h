#ifndef UTIL_MATH_H
#define UTIL_MATH_H

#include <cassert>
#include <cmath>

namespace util {

const double Epsilon          = 0.00001;
const double Pi               = 3.14159265358979; 
const double PiOver2          = 1.57079632679490;

inline constexpr bool isPowerOfTwo(int i)
{
    return (i > 0) && ((i & (i - 1)) == 0);
}

inline size_t nextPowerOfTwoMultiple(size_t i, size_t multiple)
{
    assert(util::isPowerOfTwo(multiple));
    return (i + multiple - 1) & ~(multiple- 1); 
}

bool feq(double a, double b);
bool feq(double a, double b, double epsilon);
bool fez(double a, double epsilon = Epsilon);

template <typename Real>
Real degreesToRadians(Real x)
{
    return (x*static_cast<Real>(Pi))/static_cast<Real>(180);
}

template <typename Real>
Real radiansToDegrees(Real x)
{
    return x*static_cast<Real>(180.0/Pi);
}

template <typename Num>
int sign(Num x)
{
    return (x > 0) - (x < 0);
}

// Linear interpolation. 
template <typename Real>
Real lerp(Real x, Real y, Real a)
{
    return (static_cast<Real>(1) - a)*x + a*y;
}

// Re-scales start/end to be 0.0/1.0 and determines the relative x to the new scale. Precondition 
// that start < end.
template <typename Real>
Real scale(Real start, Real end, Real x)
{
    return (x - start)/(end - start);
}

// Precondition that min < max.
template <typename Num>
Num clamp(Num x, Num min, Num max)
{
    if (x < min)
        return min;
    else if (x > max)
        return max;
    return x;
}

template <typename Real>
Real saturate(Real x)
{
    return clamp(x, static_cast<Real>(0), static_cast<Real>(1));
}

template <typename Real>
Real step(Real edge, Real x)
{
    if (x < edge)
        return static_cast<Real>(0);
    return static_cast<Real>(1);
}

template <typename Real>
Real smoothstep(Real edge1, Real edge2, Real x)
{
    x = scale(edge1, edge2, x);
    x = saturate(x);
    return x*x*(3 - 2*x);
}

template <typename Real>
Real smootherstep(Real edge1, Real edge2, Real x)
{
    x = scale(edge1, edge2, x);
    x = saturate(x);
    return x*x*x*(x*(x*6 - 15) + 10);
}

template <typename Real>
Real fract(Real x)
{
    return x - std::floor(x);
}

template <typename Real>
Real round(Real x)
{
    if (x >= 0)
        return std::floor(x + static_cast<Real>(0.5f));
    else
        return std::ceil(x - static_cast<Real>(0.5f));
}

template <typename Real>
Real truncate(Real x)
{
    if (x < 0)
        return std::ceil(x);
    else
        return std::floor(x);
}

template <typename Num>
bool inRange(Num x, Num min, Num max)
{
    return x >= min && x <= max;
}

// Need to handle float/double explcitly to handle precision issues
bool inRange(float x, float min, float max);
bool inRange(double x, double min, double max);

bool isEven(int n);
bool isOdd(int n);


} // namespace util 

#endif

