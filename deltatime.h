#pragma once


#include <chrono>
class DeltaTime
{
    std::chrono::high_resolution_clock::time_point t1;
public:
    DeltaTime()
    {
        start();
    }

    void start()
    {
        t1 = std::chrono::high_resolution_clock::now();
    }

    double stop()
    {
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = t2 - t1;
        t1 = t2;
        return diff.count();
    }
};
