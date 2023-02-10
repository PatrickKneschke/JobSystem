
#include "job_system.h"


std::unique_ptr<JobSystem> JobSystem::sInstance = nullptr;


JobSystem::JobSystem(const size_t numThreads) : mActive {false}, mNumThreads {numThreads}, mThreads {}, mJobQueue {} {

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
    sInstance->mCondVar.notify_all();

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
        return !mJobQueue.empty() || !mActive.load(std::memory_order_acquire);
    });
    
    if(!mActive.load(std::memory_order_acquire))
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