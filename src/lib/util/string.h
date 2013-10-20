#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include <string>
#include <vector>
#include <sstream>

namespace util {

template <typename T>
T toNum(const std::string& s)
{
    std::stringstream converter(s);
    
    T res;  
    return converter >> res ? res : 0;
}

template <typename T>
std::string toStr(T n)
{
    std::stringstream converter;
    converter << n;
    return converter.str();
}

// Container must be a container of std::strings.
template <typename Cont>
std::string join(Cont c, const std::string& fill)
{
    std::string acc = "";

    typename Cont::iterator i = c.begin();
    while (i != c.end()) {
        std::string s = *i;
        i++;

        if (i != c.end())
            acc += s + fill;
        else
            acc += s;
    }
    
    return acc;
}

void split(const std::string& src, std::vector<std::string>* out, const std::string& delim = " ");

std::string repeat(char src, int num);
std::string repeat(const std::string& src, int num);

std::string strPrintf(const std::string& format, ...);

}

#endif

