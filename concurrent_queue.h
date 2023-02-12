#pragma once


#include <mutex>
#include <queue>


template<typename T>
class ConcurrentQueue {

public:

    void Push(T &&in){

        std::lock_guard<std::mutex> lock{mAccess};
        mQueue.push(std::move(in));
    }

    bool Pop(T &out) {

        std::unique_lock<std::mutex> lock{mAccess};
        
        if(mQueue.empty()) {

            return false;
        }

        out = std::move(mQueue.front());
        mQueue.pop();

        return true;
    }

    bool IsEmpty() {

        std::lock_guard<std::mutex> lock{mAccess};
        return mQueue.empty();
    }

    void Clear() {

        std::lock_guard<std::mutex> lock{mAccess};
        mQueue = {};
    }

    size_t Size() {

        std::lock_guard<std::mutex> lock{mAccess};
        return mQueue.size();
    }


private:

    std::queue<T> mQueue;
    std::mutex mAccess;
};