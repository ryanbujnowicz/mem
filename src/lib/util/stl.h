#ifndef UTIL_STL_H
#define UTIL_STL_H

#include <algorithm>
#include <cassert>
#include <utility>
#include <vector>

namespace util {

/**
 * Function wrapper around delete which can be used with STL.
 *
 * This is best used in the following case:
 * std::foreach(c.begin(), c.end(), util::deleter);
 * std::foreach(c.begin(), c.end(), std::default_delete);
 *
 * Note that if ptr is an array, arrayDeleter should be used instead.
 */
struct Deleter
{
    template <class T>
    void operator()(T* ptr) const
    {
        delete ptr;
    }
};

struct ArrayDeleter
{
    template <class T>
    void operator()(T* ptr) const
    {
        delete[] ptr;
    }
};

/**
 * Erases elements from the container which are equal to t.
 */
template <typename Cont>
void removeErase(Cont& c, typename Cont::value_type t)
{
    c.erase(std::remove(c.begin(), c.end(), t), c.end());
}

/**
 * Checks if o is equal to an element of the container.
 */
template <typename Cont, typename T>
bool isElement(T o, const Cont& c)
{
    return std::find(c.begin(), c.end(), o) != c.end();
}

/**
 * For a given container, returns a vector which matches each element with the index
 * of that element in the original container.
 *
 * For example:
 * ~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * #include "util/stl.h"
 * std::vector<std::string> ss = {"hello", "there", "you"};
 * auto es = util::enumerate(ss);
 * ~~~~~~~~~~~~~~~~~~~~~~~
 *
 * In this case:
 * es[0] = (0, "hello")
 * es[1] = (1, "there")
 * es[2] = (2, "you")
 */
template <class Cont>
std::vector<std::pair<size_t, typename Cont::value_type> > enumerate(const Cont& c)
{
    size_t i = 0;
    std::vector<std::pair<size_t, typename Cont::value_type> > pairs;

    typename Cont::const_iterator iter;
    for (iter = c.begin(); iter != c.end(); ++iter) {
        pairs.push_back(std::make_pair(i, *iter));
        i++;
    }
    return pairs;
}

/**
 * Determines if the index i is within range of the container c.
 */
template <class Cont>
bool inBounds(typename Cont::size_type i, Cont& c)
{
    return i < c.size();
}

/**
 * Given a container and a list of indices, returns the elements from that container
 * matching the indices and in the order provided.
 */
template <class Indexable, class Iterable>
void index(const Indexable& items, const Iterable& indices, Indexable* out)
{
    assert(out);
    out->clear();

    typename Iterable::const_iterator iter;
    for (iter = indices.begin(); iter != indices.end(); ++iter) {
        out->push_back(items[*iter]);
    }
}

/**
 * Given two containers, returns a new container of pairs of each corresponding element.
 */
template <class T>
std::vector<std::pair<T,T> > zip(const std::vector<T>& u, const std::vector<T>& v)
{
    std::vector<std::pair<T,T> > zipped;

    size_t minSize = std::min(u.size(), v.size());
    for (size_t i = 0; i < minSize; ++i)
        zipped.push_back(std::make_pair(u[i], v[i]));

    return zipped;
}

}

#endif

