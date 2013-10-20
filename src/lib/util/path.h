#ifndef UTIL_PATH_H
#define UTIL_PATH_H

#include <string>

namespace util {

std::string getDirName(const std::string& path);
std::string getBaseName(const std::string& path);

}

#endif

