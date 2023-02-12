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

    enum class Priority : uint8_t {

        HIGH = 1, NORMAL, LOW, COUNT
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
    static auto Submit(Func&& func, Args&&... args) {

        auto callable = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);

        using ResultType = std::result_of_t<decltype(callable)()>;
        using Task = std::packaged_task<ResultType()>;

        Task task(std::move(callable));
        std::future<ResultType> result = task.get_future();

        sInstance->mJobQueue.Push( std::make_unique<JobDecl<Task>>(std::move(task)) );
        
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

    // job queue
    ConcurrentQueue<std::unique_ptr<IJobDecl>> mJobQueue;
};