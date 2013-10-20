#ifndef MEM_REGIONREPORTER_H
#define MEM_REGIONREPORTER_H

#include <sstream>

#include "mem/tracking.h"

namespace {
    
std::string makeMemoryLeakSection(const mem::NoTracking& tracker)
{
    return "";
}

std::string makeMemoryLeakSection(const mem::CountTracking& tracker)
{
    std::stringstream ss;
    ss << "\tUnreleased Memory Allocations: " 
        << tracker.getAllocationCount() " allocs "
        << "(" << tracker.getAllocatedSize() << " bytes)\n";
    return ss.str();
}

std::string makeMemoryLeakSection(const mem::SourceTracking& tracker)
{
    return "\tSource Tracking\n";
}

std::string makeMemoryLeakSection(const mem::CallStackTracking& tracker)
{
    return "\tCall stack Tracking\n";
}

}

namespace mem {

template <class Region>
std::string makeRegionReport(const std::string& name, const Region& region)
{
    std::stringstream ss;
    ss << "Region " << name << " report:\n";
    ss << makeStatsSection();
    ss << makeMemoryLeakSection(region.getTrackerPolicy());
    return ss.str();
}

} // namespace mem

#endif

