#pragma once


#include "concurrent_queue.h"

#include <algorithm>
#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>


class JobSystem {

public:

    enum Priority : uint8_t {

        HIGH = 0, NORMAL, LOW, COUNT
    };


private:

    class IJobDecl {

        public:
            virtual void run() = 0;
    };


    template<typename Task>
    class JobDecl : public IJobDecl {

        public:
            JobDecl(Task &&task) : mTask {std::forward<Task>(task)} {}

            void run() override {

                mTask();
            }

        private:
            Task mTask;
    };


public:

    static void StartUp(size_t numThreads = 0);
    static void ShutDown();
    static void ClearJobs();

    ~JobSystem();    

    template <typename Func, typename... Args>
    static auto Submit(const Priority priority, Func&& func, Args&&... args) {

        auto callable = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);

        using ResultType = std::result_of_t<decltype(callable)()>;
        using Task = std::packaged_task<ResultType()>;

        Task task(std::move(callable));
        std::future<ResultType> result = task.get_future();

        switch (priority)
        {
        case Priority::HIGH:
            sInstance->mJobQueueHigh.Push( std::make_unique<JobDecl<Task>>(std::move(task)) );
            break;
        case Priority::NORMAL:
            sInstance->mJobQueueNormal.Push( std::make_unique<JobDecl<Task>>(std::move(task)) );
            break;
        case Priority::LOW:
            sInstance->mJobQueueHigh.Push( std::make_unique<JobDecl<Task>>(std::move(task)) );
            break;        
        default:
            break;
        }
        
        return result;
    }


private:

    JobSystem(const size_t numThreads);

    void WorkerThread();

    // job system instance
    static std::unique_ptr<JobSystem> sInstance;

    // job system initialized and ready for job submissions
    std::atomic_bool mActive;

    // thread pool
    size_t mNumThreads;
    std::vector<std::thread> mThreads;

    // job queues
    ConcurrentQueue<std::unique_ptr<IJobDecl>> mJobQueueHigh;
    ConcurrentQueue<std::unique_ptr<IJobDecl>> mJobQueueNormal;
    ConcurrentQueue<std::unique_ptr<IJobDecl>> mJobQueueLow;
};