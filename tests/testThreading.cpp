#include <gtest/gtest.h>

#include <thread>
#include <mutex>

#include "mem/threading.h"

mem::MultiThreaded<std::mutex> mutexThreading;
int globalInt;

void func1()
{
    mutexThreading.begin();
    globalInt++;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    globalInt++;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    globalInt++;
    mutexThreading.end();
}

void func2()
{
    while (true) {
        mutexThreading.begin();
        if (globalInt > 0) {
            globalInt *= 4;
            mutexThreading.end();
            break;
        }
        mutexThreading.end();
    }
}

TEST(Threading, SingleThreaded)
{
    // This is a noop
    mem::SingleThreaded threading;
    threading.begin();
    threading.end();
}

TEST(Threading, MultiThreadedMutex)
{
    globalInt = 0;
    std::thread f1(func1);
    std::thread f2(func2);
    f1.join();
    f2.join();
    EXPECT_EQ(12, globalInt);
}

