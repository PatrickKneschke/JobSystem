#pragma once


#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>


class IJobDecl {

public:
    virtual void run() = 0;
};

template<typename Task>
class JobDecl : public IJobDecl {

public:
    JobDecl(Task &&task_) : task {std::forward<Task>(task_)} {}

    void run() override {

        task();
    }

private:
    Task task;
};


class JobSystem {

public:

    JobSystem(const size_t numThreads = 0);
    ~JobSystem();

    void StartUp();
    void ShutDown();

    template <typename Func, typename... Args>
    auto Submit(Func&& func, Args&&... args) {

        auto callable = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);

        using ResultType = std::result_of_t<decltype(callable)()>;
        using Task = std::packaged_task<ResultType()>;

        Task task(std::move(callable));
        std::future<ResultType> result = task.get_future();

        PushJob( std::make_unique<JobDecl<Task>>(std::move(task)) );
        
        return result;
    }

private:

    void WorkerThread();

    void PushJob(std::unique_ptr<IJobDecl> &&job);
    bool PopJob(std::unique_ptr<IJobDecl> &out);
    void ClearJobs();


    // job system initialized and ready for job submissions
    std::atomic_bool mActive;

    // thread pool
    size_t mNumThreads;
    std::vector<std::thread> mThreads;

    // job queue
    std::queue<std::unique_ptr<IJobDecl>> mJobQueue;
    std::mutex mQueueAccess;
    std::condition_variable mCondVar;
};