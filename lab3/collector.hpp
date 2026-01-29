#pragma once
#include "safequeue.hpp"
#include "safeprint.hpp"
#include "fileprocessor.hpp"
#include <mutex>
#include <vector>
#include <map>

class Collector
{
private:
    ThreadSafeQueue<FileInfo>& result_queue;
    std::vector<FileInfo> results;
    int expected_result;
    std::mutex results_mutex;

public:
    Collector(ThreadSafeQueue<FileInfo>& queue, int expected)
        : result_queue(queue), expected_result(expected) {}
    void collect()
    {
        int collected  = 0;
        while(collected < expected_result)
        {
            auto result = result_queue.pop();
            if(result.has_value())
            {
                std::lock_guard<std::mutex> lock(results_mutex);
                results.push_back(result.value());
                collected++;
                SafePrint::print("Collected result " + std::to_string(collected) + "/"
                 + std::to_string(expected_result) + " --> " + result.value().file + "\n");
            }
        }
        std::cout << "ALL RESULTS COLLECTED\n";
        print_total_info();
    } 
    
    void print_total_info()
    {
        size_t total_files = results.size();
        size_t total_words = 0;
        size_t total_lines = 0;
        size_t total_size = 0;

        for(const auto& res : results)
        {
            total_words += res.word_count;
            total_lines += res.line_count;
            total_size += res.file_size;
        }

        SafePrint::print("Total files: " + std::to_string(total_files) + "\n");
        SafePrint::print("Total size: " + std::to_string(total_size) + " bytes\n");
        SafePrint::print("Total lines: " + std::to_string(total_lines) + "\n");
        SafePrint::print("Total words: " + std::to_string(total_words) + "\n\n");

    }
};

