#ifndef MEM_SOURCEINFO_H
#define MEM_SOURCEINFO_H

#include <string>

namespace mem {

/**
 * Contains information identifying the source of an allocation.
 * 
 * This is used by the memory region system to track where an allocation has originated from.
 */
struct SourceInfo
{
    SourceInfo(const std::string& filename = "", size_t lineNumber = 0) :
        filename(filename),
        lineNumber(lineNumber)
    {
    }

    std::string filename;   
    size_t lineNumber;   
};

} // namespace mem

#endif

