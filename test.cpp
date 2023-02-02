
#include "job_system.h"


#include <chrono>
#include <iostream>
#include <string>
#include <vector>


using Clock = std::chrono::high_resolution_clock;
using Time  = std::chrono::time_point<std::chrono::high_resolution_clock>;


int64_t duration(Time start, Time end) {

    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}


int test() {

    std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}


int main(int argc, char** argv) {

    Clock clock;
    Time start, end;

    int N = 100;

    std::vector<int> resSingle(N);    
    start = clock.now();
    for (size_t i = 0; i < N; i++)
    {
        resSingle[i] = test();
    }

    end = clock.now();

    std::cout << "Single thread time : " << duration(start, end) << '\n'; 
    
    std::vector<std::future<int>> resMulti(N);

    JobSystem jobSys;
    jobSys.StartUp();

    start = clock.now();
    for (size_t i = 0; i < N; i++)
    {
        resMulti[i] = jobSys.Submit(test);
    }
    for (size_t i = 0; i < N; i++)
    {
        resMulti[i].get();
    }
    
    end = clock.now();

    std::cout << "Multi thread time : " << duration(start, end) << '\n'; 

    jobSys.ShutDown();

    return 0;
}