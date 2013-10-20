#include "mem/region.h"
using namespace mem;

#include <cassert>

#include "util/singleton.h"

namespace {

/**
 * Stores global memory regions.
 *
 * Public access to this class is done through the functions:
 *
 *     mem::registerRegion
 *     mem::getRegion
 *     mem::setDefaultRegion
 *
 * Regions are associated with IDs and can then be retrieved using that ID.
 * A special DefaultRegion ID exists which can be changed to point to any
 * other ID. 
 */
class RegionRegistry : public util::Singleton<RegionRegistry>
{
    SINGLETON(RegionRegistry);

public:

    /**
     * Registers the given region with the given ID number. This ID can then be used retrieve
     * the registered region.
     *
     * @pre No region with the given id may already be registered.
     * @pre id must be >= 0 and not exceed (MaxRegions - 1)
     */
    void registerRegion(mem::RegionBase& region, const int id)
    {
        assert(id > DefaultRegion && id < RegionRegistry::MaxRegions);
        assert(!_regions[id]);
        _regions[id] = &region;
    }

    /**
     * Retrieves a previously registered region.
     *
     * @pre id must refer to a previously regstered region.
     */
    RegionBase& getRegion(int id)
    {
        if (id == DefaultRegion) {
            id = _defaultRegion;
        }
        assert(id >= 0 && id < MaxRegions && _regions[id]);
        return *_regions[id];
    }

    void setDefaultRegion(int id)
    {
        assert(id> DefaultRegion);
        _defaultRegion = id;
    }

private:
    virtual void init() override
    {
        _defaultRegion = 1;
        std::fill(std::begin(_regions), std::end(_regions), nullptr);
    }

    static const size_t MaxRegions = 8;
    RegionBase* _regions[MaxRegions];

    int _defaultRegion;
};

}

void mem::registerRegion(RegionBase& region, const int id)
{
    RegionRegistry::getInstance().registerRegion(region, id);
}

mem::RegionBase& mem::getRegion(int id)
{
    return RegionRegistry::getInstance().getRegion(id);
}

void mem::setDefaultRegion(int id)
{
    RegionRegistry::getInstance().setDefaultRegion(id);
}

