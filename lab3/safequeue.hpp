#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template<typename T>
class ThreadSafeQueue
{
private:
    std::queue<T> queue;
    mutable std::mutex mutex;
    std::condition_variable condition_var;
    bool shd = false;

public:

    void push(T object)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (shd)
            return; 
        queue.push(std::move(object));
        condition_var.notify_one();
    
    }

    std::optional<T> pop()
    {
        std::unique_lock<std::mutex> lock(mutex);
        condition_var.wait(lock, [this]() 
        {return !queue.empty() || shd;});

        if(queue.empty())
            return std::nullopt;
        T obj = std::move(queue.front());
        queue.pop();
        return obj;
    }

    void shutdown()
    {
        std::lock_guard<std::mutex> lock(mutex);
        shd = true;
        condition_var.notify_all();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.empty();
    }
};