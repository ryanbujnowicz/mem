#ifndef UTIL_STACKTRACE_H
#define UTIL_STACKTRACE_H

#include <cassert>
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>

#include "util/path.h"
#include "util/platform.h"

namespace util {

static const size_t MaxCallStackLevels = 32;

/**
 * Retrieves a stack trace from the system.
 *
 * The resulting trace is put in _result_ which must be pre-allocated to at least MaxCallStackLevels
 * entries. _result_ will be filled with strings allocated with malloc, they will need to be free'd
 * by the caller.
 */
inline char** getStackTrace(int* nEntries, int skip = 1)
{
    static void* callstack[MaxCallStackLevels];
    static char buffer[1024];

    int nFrames = backtrace(callstack, MaxCallStackLevels);
    char** symbols = backtrace_symbols(callstack, nFrames);
    assert(symbols);

    char** stackTrace = (char**)malloc(sizeof(char*)*nFrames);

    // TODO: It would be great to get complete file name and line numbers using
    // llvm-symbolizer or addr2line.
    for (int i = skip; i < nFrames; i++) {
        Dl_info info;
        if (dladdr(callstack[i], &info) && info.dli_sname) {
            char* demangled = nullptr;
            int status = -1;

            if (info.dli_sname[0] == '_') {
                demangled = abi::__cxa_demangle(info.dli_sname, nullptr, 0, &status);
            }

            char* symbol = symbols[i];
            if (status == 0) {
                symbol = demangled;
            } else if (info.dli_sname > 0) {
                symbol = const_cast<char*>(info.dli_sname);
            }

            snprintf(buffer, sizeof(buffer), "%-3d %*p %s + %zd",
                    i, (int)sizeof(void*)*2, callstack[i],
                    symbol,
                    (char*)callstack[i] - (char*)info.dli_saddr);
            free(demangled);
        } else {
            snprintf(buffer, sizeof(buffer), "%-3d %*p %s",
                i, (int)sizeof(void*)*2, callstack[i], symbols[i]);    
        }

        int bufferLen = strlen(buffer);
        char* newSymbol = (char*)malloc(sizeof(char)*bufferLen + 1); 
        strncpy(newSymbol, buffer, bufferLen + 1);
        stackTrace[i - skip] = newSymbol;
    }

    free(symbols);
    assert(nEntries);
    *nEntries = nFrames - skip;
    return stackTrace;
}

} // namespace util

#endif

