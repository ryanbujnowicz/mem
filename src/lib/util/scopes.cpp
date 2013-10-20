#include "util/scopes.h"
using namespace util;

#include <string>

std::string util::stripScopes(const std::string& name)
{
    std::string::size_type sep = name.find_last_of(":");
    if (sep == std::string::npos) {
        return name;
    }
    return name.substr(sep + 1);
}

