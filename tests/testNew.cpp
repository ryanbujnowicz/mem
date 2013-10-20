#include <gtest/gtest.h>

#include "mem/boundsChecking.h"
#include "mem/mallocAllocator.h"
#include "mem/marking.h"
#include "mem/new.h"
#include "mem/region.h"
#include "mem/threading.h"
#include "mem/tracking.h"

typedef mem::Region<
    mem::MallocAllocator,
    mem::SingleThreaded,
    mem::NoBoundsChecking,
    mem::NoTracking,
    mem::NoMarking> 
        SimpleMallocRegion;

class MyClass
{
public:
    MyClass(int x, int y) :
        _x(x),
        _y(y)
    {
    }

    MyClass() :
        _x(5),
        _y(10)
    {
    }

    int _x;
    int _y;
};

class RefClass
{
public:
    RefClass(int& ref) :
        ref(ref)
    {
        ref++;
    }

    ~RefClass()
    {
        ref--;
    }

    int& ref;
};

TEST(New, NewPOD)
{
    SimpleMallocRegion region;
    int* mc = new (region, __FILE__, __LINE__) int;
    bool* mc2 = new (region, __FILE__, __LINE__) bool;
    EXPECT_TRUE(mc != nullptr);
    EXPECT_TRUE(mc2 != nullptr);
}

TEST(New, NewNonPodSimple)
{
    SimpleMallocRegion region;
    MyClass* mc = new (region, __FILE__, __LINE__) MyClass;
    EXPECT_EQ(5, mc->_x);
    EXPECT_EQ(10, mc->_y);
}

TEST(New, NewNonPodConstructor)
{
    SimpleMallocRegion region;
    MyClass* mc = new (region, __FILE__, __LINE__) MyClass(100, 200);
    EXPECT_EQ(100, mc->_x);
    EXPECT_EQ(200, mc->_y);
}

TEST(New, NewArrayPod)
{
    SimpleMallocRegion region;
    int* mc = mem::newArray<int>(region, 4, __FILE__, __LINE__);
    char* mc2 = mem::newArray<char>(region, 16, __FILE__, __LINE__);
    EXPECT_TRUE(mc != nullptr);
    EXPECT_TRUE(mc2 != nullptr);
}

TEST(New, NewArrayNonPod)
{
    SimpleMallocRegion region;
    MyClass* mc = mem::newArray<MyClass>(region, 8, __FILE__, __LINE__);
    MyClass* mc2 = mem::newArray<MyClass>(region, 128, __FILE__, __LINE__);
    EXPECT_TRUE(mc != nullptr);
    EXPECT_TRUE(mc2 != nullptr);
}

TEST(New, DeletePod)
{
    SimpleMallocRegion region;
    int* mc = new (region, __FILE__, __LINE__) int;
    bool* mc2 = new (region, __FILE__, __LINE__) bool;
    mem::deleteMem(mc2, region);
    mem::deleteMem(mc, region);
}

TEST(New, DeleteNonPod)
{
    // Tests for compilation really
    SimpleMallocRegion region;
    MyClass* mc = new (region, __FILE__, __LINE__) MyClass;
    mem::deleteMem(mc, region);

    // Ensure the destructor is called
    int val = 5;
    RefClass* mc2 = new (region, __FILE__, __LINE__) RefClass(val);
    EXPECT_EQ(6, val);
    mem::deleteMem(mc2, region);
    EXPECT_EQ(5, val);
}

TEST(New, DeleteArrayPod)
{
    SimpleMallocRegion region;
    int* mc = mem::newArray<int>(region, 4, __FILE__, __LINE__);
    char* mc2 = mem::newArray<char>(region, 16, __FILE__, __LINE__);
    mem::deleteArray(mc2, region);
    mem::deleteArray(mc, region);
}

TEST(New, DeleteArrayNonPod)
{
    SimpleMallocRegion region;
    MyClass* mc = mem::newArray<MyClass>(region, 8, __FILE__, __LINE__);
    MyClass* mc2 = mem::newArray<MyClass>(region, 128, __FILE__, __LINE__);
    mem::deleteArray(mc2, region);
    mem::deleteArray(mc, region);
}

TEST(New, Macros)
{
    SimpleMallocRegion region;

    int* a = _MEM_NEW_IMPL(int, region);
    EXPECT_TRUE(a != nullptr);

    MyClass* mc = _MEM_NEW_IMPL(MyClass, region)(1, 2);
    EXPECT_TRUE(mc != nullptr);
    EXPECT_EQ(1, mc->_x);
    EXPECT_EQ(2, mc->_y);

    _MEM_DELETE_IMPL(a, region);
    _MEM_DELETE_IMPL(mc, region);

    int* as = _MEM_NEW_ARRAY_IMPL(int[4], region);
    MyClass* mcs = _MEM_NEW_ARRAY_IMPL(MyClass[101], region);
    EXPECT_TRUE(as != nullptr);
    EXPECT_TRUE(mcs != nullptr);

    _MEM_DELETE_ARRAY_IMPL(as, region);
    _MEM_DELETE_ARRAY_IMPL(mcs, region);
}

