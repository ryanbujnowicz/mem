#ifndef UTIL_SCOPES_H
#define UTIL_SCOPES_H

#include <cmath>
#include <string>

namespace util {

/**
 * Strips namespace scopes from input string.
 *
 * e.g. 
 * stripScopes("some::id::with::scopes") -> "scopes"
 */
std::string stripScopes(const std::string& name);

}


#endif

