#ifndef UTIL_SINGLETON_H
#define UTIL_SINGLETON_H

#include <cstdlib>
#include <mutex>

namespace util {

#define SINGLETON(Class)                 \
    Class() { init(); };    \
    friend class util::Singleton<Class>;      \

/**
 * Globally-accessible single-instance class.
 *
 * A class which inherits from Singleton will have a single, globally-accessible instance.
 * Use as so:
 *
 * class MyClass : public util::Singleton<MyClass>
 * {
 *     SINGLETON(MyClass);
 *     ...
 * };
 *
 */
template<class T>
class Singleton
{
public:
    static T& getInstance()
    {
        static std::once_flag flag;
        std::call_once(flag, [](){ 
            _instance = Singleton<T>::_getInstance();
        });
        return *_instance;
    }

protected:
    Singleton() {};
    virtual ~Singleton()
    {
        _instance = nullptr;
    }

    /**
     * This function gets called when the singleton is initialized. Override to provide specific
     * initialization behaviour.
     */
    virtual void init()
    {
    }

private:
    static T* _getInstance()
    {
        static T instance;
        return &instance;
    }


private:
    Singleton(const Singleton& rhs);
    Singleton& operator=(const Singleton& rhs);

    static T* _instance;
};

template <class T>
T* Singleton<T>::_instance = 0;

}

#endif

