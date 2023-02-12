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


/* @brief Singleton class to manage job execution. Stores job declarations in job queues based on priority.
 *
 * @class
 */
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

    /* @brief Initializes the job system instance and creates the threadpool.
     *
     * @param numThreads    The number of threads in the pool. If zero the hardware concurrency is chosen by default.
     */
    static void StartUp(size_t numThreads = 0);

    /* @brief Joins all running threads and destroys job system instance.
     */
    static void ShutDown();

    /* @brief Clears all job queues.
     */
    static void ClearJobs();

    ~JobSystem();    

    /* @brief Creates a job declaration for the given task and adds it to a job queue based on priority. 
     *        Wraps the function into a std::packaged_task and returns the associated std::future.
     *
     * @param priority    The jobs priority (high, normal or low).
     * @param func    The function to execute.
     * @param args    The arguments to pass tot he function.
     * 
     * @return std::future of teh return tpe of the task.
     */
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

    /* @brief Runs on each thread until job system is shut down.
     *        Looks for a task to execute from high to low priority.
     */
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