
#include "job_system.h"


JobSystem::JobSystem(const size_t numThreads) : mActive {false}, mNumThreads {numThreads}, mThreads {}, mJobQueue {} {

    if (mNumThreads == 0)
    {
        mNumThreads = std::max(std::thread::hardware_concurrency(), 2u) - 1u;
    }    
}


JobSystem::~JobSystem() {

}


void JobSystem::StartUp() {

    mActive = true;
    try
    {
        for(size_t i = 0; i < mNumThreads; i++)
        {
            mThreads.emplace_back( &JobSystem::WorkerThread, this );
        }
    }
    catch(...)
    {
        ShutDown();
        throw std::runtime_error("Thread pool creation failed!");
    }
}


void JobSystem::ShutDown() {
    
    mActive = false;
    mCondVar.notify_all();

    for(auto& thread : mThreads)
    {
        if(thread.joinable())
        {
            thread.join();
        }
    }
}


void JobSystem::WorkerThread() {

    std::unique_ptr<IJobDecl> job;
    while (mActive)
    {
        if (PopJob(job))
        {
            job->run();
        }
    }
}


void JobSystem::PushJob(std::unique_ptr<IJobDecl> &&job) {

    std::lock_guard<std::mutex> lock{mQueueAccess};
    mJobQueue.push(std::move(job));
    mCondVar.notify_one();
}
    
    
bool JobSystem::PopJob(std::unique_ptr<IJobDecl> &out) {

    std::unique_lock<std::mutex> lock{mQueueAccess};
    mCondVar.wait(lock, [this]()
    {
        return !mJobQueue.empty() || !mActive;
    });
    
    if(!mActive)
    {
        return false;
    }

    out = std::move(mJobQueue.front());
    mJobQueue.pop();

    return true;
}


void JobSystem::ClearJobs() {

    std::lock_guard<std::mutex> lock{mQueueAccess};
    mJobQueue = {};
    mCondVar.notify_all();
}