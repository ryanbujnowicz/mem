#ifndef UTIL_PLATFORM_H
#define UTIL_PLATFORM_H

#ifdef _WIN32
#define OS_WINDOWS
#elif __linux
#define OS_LINUX
#elif __APPLE__
    #define OS_DARWIN

    #include "TargetConditionals.h"
    #if TARGET_OS_IPHONE
    #define OS_IPHONE
    #elif TARGET_OS_IPAD
    #define OS_IPAD
    #elif TARGET_OS_MAC
    #define OS_MACOSX
    #else
    #error Unknown platform
    #endif
#else
    #error Unknown platform
#endif

// Enable certain functionality.
#if defined(OS_MACOSX) || defined(OS_IPHONE) || defined(OS_IPAD)|| defined(OS_LINUX)
#define HAS_SYS_TIME
#endif

#endif

