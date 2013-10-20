#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

#include "mem/mallocAllocator.h"
#include "mem/boundsChecking.h"
#include "mem/region.h"
#include "mem/marking.h"
#include "mem/tracking.h"
#include "mem/threading.h"
#include "mem/stlAdapter.h"

typedef mem::Region<
    mem::MallocAllocator,
    mem::SingleThreaded,
    mem::NoBoundsChecking,
    mem::NoTracking,
    mem::NoMarking> 
        SimpleMallocRegion;
SimpleMallocRegion mallocRegion;

template <class T, class A1, class A2>
bool operator==(
        const std::vector<T, A1>& v1,
        const std::vector<T, A2>& v2)
{
    return v1.size() == v2.size() &&
        std::equal(v1.begin(), v1.end(), v2.begin());
}

template <class TT, class A1, class A2>
bool operator==(
        const std::basic_string<char, TT, A1>& s1,
        const std::basic_string<char, TT, A2>& s2)
{
    return s1.size() == s2.size() &&
        std::equal(s1.begin(), s1.end(), s2.begin());
}

template <class A1, class A2>
bool mapEq(
        const std::map<const std::string, int, std::less<const std::string>, A1>& m1,
        const std::map<const std::string, int, std::less<const std::string>, A2>& m2)
{
    return m1.size() == m2.size() &&
        std::equal(m1.begin(), m1.end(), m2.begin());
}

TEST(StlAdapter, StlVector)
{
    std::vector<int, mem::StlAdapter<int>> 
        vec({5, 7, 8}, mem::StlAdapter<int>(mallocRegion));
    std::vector<int> vecBase = {5, 7, 8};
    EXPECT_TRUE(vecBase == vec);

    vec.erase(std::begin(vec) + 1);
    vecBase.erase(std::begin(vecBase) + 1);
    EXPECT_TRUE(vecBase == vec);

    vec.push_back(110);
    vecBase.push_back(110);
    EXPECT_TRUE(vecBase == vec);

    vec.insert(std::begin(vec) + 1, 12);
    vecBase.insert(std::begin(vecBase) + 1, 12);
    EXPECT_TRUE(vecBase == vec);
}

TEST(StlAdapter, StlString)
{
    typedef std::basic_string<char, std::char_traits<char>, mem::StlAdapter<char>> 
        String;
    String s1("hello there xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 
            mem::StlAdapter<char>(mallocRegion));
    std::string s2 = "hello there xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    EXPECT_TRUE(s2 == s1);

    s1.erase(1, 6);
    s2.erase(1, 6);
    EXPECT_TRUE(s2 == s1);

    s1.insert(0, "start ");
    s2.insert(0, "start ");
    EXPECT_TRUE(s2 == s1);
}

TEST(StlAdapter, StlMap)
{
    typedef std::map<const std::string, int, std::less<const std::string>, 
            mem::StlAdapter<std::pair<const std::string,int>>> Map;

    Map m1((mem::StlAdapter<std::pair<const std::string,int>>(mallocRegion)));

    m1["one"] = 1;
    m1["two"] = 2;

    EXPECT_TRUE(m1.find("one") != m1.end());
    EXPECT_TRUE(m1.find("two") != m1.end());

    m1.erase("one");
    EXPECT_TRUE(m1.find("one") == m1.end());

    m1.insert(std::make_pair("three", 3));
    EXPECT_EQ(3, m1["three"]);
}

