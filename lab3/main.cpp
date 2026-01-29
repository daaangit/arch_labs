#include <thread>
#include "fileprocessor.hpp"
#include "producer.hpp"
#include "consumer.hpp"
#include "collector.hpp"
#include "safequeue.hpp"

int main()
{
    std::vector<std::string> files = {
        "files/test1.txt", "files/test2.txt", "files/test3.txt",
        "files/test4.txt", "files/test5.txt", "files/test6.txt",
        "files/test7.txt", "files/test8.txt", "files/test9.txt",
        "files/test10.txt"
    };
    
    ThreadSafeQueue<std::string> task_queue;
    ThreadSafeQueue<FileInfo> result_queue;
    Producer producer(task_queue, files);
    
    const unsigned CONSUMERS_NUMBER = 3;
    std::vector<std::thread> consumer_threads;

    for(unsigned i = 0; i < CONSUMERS_NUMBER; ++i)
    {
        consumer_threads.emplace_back([&task_queue, &result_queue, i]()
        {
            Consumer consumer(task_queue, result_queue, i + 1);
            consumer.run();
        });
    }
    
    Collector collector(result_queue, files.size());
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::thread producer_thread(&Producer::run, &producer);
    collector.collect();
    producer_thread.join();

    for(auto& thread : consumer_threads)
        thread.join();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    SafePrint::print("Processed successfully\n");
    std::cout << "Total time: " << duration.count() << " ms\n";
    std::cout << "Threads used: " << CONSUMERS_NUMBER << "\n";
    
    return 0;
}

