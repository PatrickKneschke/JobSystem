#pragma once


#include <mutex>
#include <queue>


/* @brief Thread safe queue using a std::mutex to protect queue operations.
 */
template<typename T>
class ConcurrentQueue {

public:

    /* @brief Adds an object to the queue.
     *
     * @param in    The object to add.
     */
    void Push(T &&in){

        std::lock_guard<std::mutex> lock{mAccess};
        mQueue.push(std::move(in));
    }

    /* @brief Tries to get an object from the queue.
     *
     * @param out    Reference to the the object to store the retrieved object in.
     * 
     * @return Whether an object was retrieved from the queue.
     */
    bool Pop(T &out) {

        std::unique_lock<std::mutex> lock{mAccess};
        
        if(mQueue.empty()) {

            return false;
        }

        out = std::move(mQueue.front());
        mQueue.pop();

        return true;
    }

    /* @brief Checks if the queue is empty.
     */
    bool IsEmpty() {

        std::lock_guard<std::mutex> lock{mAccess};
        return mQueue.empty();
    }

    /* @brief Resets the queue to an emty state.
     */
    void Clear() {

        std::lock_guard<std::mutex> lock{mAccess};
        mQueue = {};
    }

    /* @brief The number of elements in the queue.
     */
    size_t Size() {

        std::lock_guard<std::mutex> lock{mAccess};
        return mQueue.size();
    }


private:

    std::queue<T> mQueue;
    std::mutex mAccess;
};