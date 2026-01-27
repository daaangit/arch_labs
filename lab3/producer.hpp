#pragma once
#include "safequeue.hpp"
#include "safeprint.hpp"
#include <vector>
#include <string>
#include <filesystem>
#include <iostream>

class Producer
{
private:
    ThreadSafeQueue<std::string>& task_queue;
    std::vector<std::string> files;

public:
    Producer(ThreadSafeQueue<std::string>& queue, const std::vector<std::string>& files):
    task_queue(queue), files(files) {}

    void run()
    {
        for (const std::string& file : files)
        {
            task_queue.push(file);
            SafePrint::print("Producer added: " + file + "\n");
        }
        task_queue.shutdown();
        SafePrint::print("Producer: all tasks added\n");
    }
};