#ifndef UTIL_DEBUG_H
#define UTIL_DEBUG_H

#include <iostream>
#include <cstdio>
#include <cstdlib>

#include "util/path.h"

#if defined(LINUX) || defined(__APPLE__)
#define HALT() abort()
#elif WIN32
#define HALT() __debugbreak()
#else
#error Not implemented
#endif

#if defined(DEBUG) && DEBUG==1
#define DEBUG_HEADER printf("DEBUG[%s:%d] ", util::getBaseName(__FILE__).c_str(), __LINE__)
#define DEBUG_PRINTF(fmt, ...) DEBUG_HEADER; printf(fmt, __VA_ARGS__) 
#define DEBUG_MSG(val) DEBUG_HEADER; std::cout << val << std::endl
#define DEBUG_VAR(var) DEBUG_HEADER; std::cout << (#var) << " = " << var << std::endl
#else
#define DEBUG_PRINTF(fmt, ...)
#define DEBUG_MSG(val)
#define DEBUG_VAR(var)
#endif

#endif

