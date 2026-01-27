#pragma once
#include "fileprocessor.hpp"
#include "safeprint.hpp"
#include <functional>

class Consumer
{
private:
    ThreadSafeQueue<std::string>& task_queue;
    ThreadSafeQueue<FileInfo>& result_queue;
    FileProcessor fproc;
    int thread_id;
public:
    Consumer(ThreadSafeQueue<std::string>& tasks,
        ThreadSafeQueue<FileInfo>& results, int thr_id)
        : task_queue(tasks), result_queue(results), thread_id(thr_id) {}

    void run()
    {
        while(true)
        {
            auto task = task_queue.pop();
            if(!task.has_value())
            {
                SafePrint::print("Consumer " + std::to_string(thread_id) + " finished\n");
                break;
            }
            std::string filename = task.value();
            SafePrint::print("Consumer " + std::to_string(thread_id) + " -> " + filename + "\n");
            FileInfo result = fproc.process(filename);
            result_queue.push(result);
        }
    }
};
