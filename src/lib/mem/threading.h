#ifndef MEM_THREADING_H
#define MEM_THREADING_H

namespace mem {

/**
 * Fulfills ThreadPolicy.
 */
class SingleThreaded
{
public:
    inline void begin() const {}
    inline void end() const {}
};

/**
 * The SyncPrim concept allows us to define specifically what method
 * is used later on, as long as it has a lock/unlock method. This is typically
 * something like a Mutex, but could also be a SpinLock, LockFree, etc.
 */
template <class SyncPrim>
class MultiThreaded
{
public:
    inline void begin()
    {
        _syncPrim.lock();
    }

    inline void end()
    {
        _syncPrim.unlock();
    }

private:
    SyncPrim _syncPrim;
};

} // namespace mem

#endif

