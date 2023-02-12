
#include "job_system.h"


std::unique_ptr<JobSystem> JobSystem::sInstance = nullptr;


JobSystem::JobSystem(const size_t numThreads) : mActive {false}, mNumThreads {numThreads} {

    if (mNumThreads == 0)
    {
        mNumThreads = std::max(std::thread::hardware_concurrency(), 2u) - 1u;
    }
}


JobSystem::~JobSystem() {

}


void JobSystem::StartUp(size_t numThreads) {

    if (!sInstance)
    {
        sInstance.reset( new JobSystem(numThreads) );
    }
    
    sInstance->mActive.store(true, std::memory_order_release);
    try
    {
        for(size_t i = 0; i < sInstance->mNumThreads; i++)
        {
            sInstance->mThreads.emplace_back( &JobSystem::WorkerThread, sInstance.get() );
        }
    }
    catch(...)
    {
        ShutDown();
        throw std::runtime_error("Thread pool creation failed!");
    }
}


void JobSystem::ShutDown() {
    
    sInstance->mActive.store(false, std::memory_order_release);

    for(auto& thread : sInstance->mThreads)
    {
        if(thread.joinable())
        {
            thread.join();
        }
    }

    sInstance.reset(nullptr);
}


void JobSystem::WorkerThread() {

    std::unique_ptr<IJobDecl> job;
    while (mActive.load(std::memory_order_acquire))
    {
        if (mJobQueueHigh.Pop(job))
        {
            job->run();
        }
        else if (mJobQueueNormal.Pop(job))
        {
            job->run();
        }
        else if (mJobQueueLow.Pop(job))
        {
            job->run();
        }
    }
}


void JobSystem::ClearJobs() {

    sInstance->mJobQueueHigh.Clear();
    sInstance->mJobQueueNormal.Clear();
    sInstance->mJobQueueLow.Clear();
}