#pragma once
#include <string>
#include <map>
#include <fstream>
#include <algorithm>
#include <iostream>


struct FileInfo
{
    std::string file;
    size_t word_count = 0, line_count = 0, file_size = 0;
    std::string longest_word;
};

class FileProcessor
{
public:
    FileInfo process(const std::string& filename)
    {
        FileInfo info;
        info.file = filename;
        
        std::ifstream file(filename);
        if(!file.is_open())
        {
            std::cout << "File not found : " << filename << "\n";
            return info;
        }
        file.seekg(0, std::ios::end);
        info.file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::string line;

        while(std::getline(file, line)) 
        {
            info.line_count++;
            bool char_in_word = false;
            std::string current = "";

            for(char c : line)
            {
                if(c == ' ' || c == '\t')
                {
                    if(char_in_word)
                    {
                        info.word_count++;
                        if(current.length() > info.longest_word.length())
                            info.longest_word = current;
                        char_in_word = false;
                        current = "";
                    }
                }
                else
                {
                    char_in_word = true;
                    current += c;
                }
            }
            
            if(char_in_word)
            {
                info.word_count++;
                if(current.length() > info.longest_word.length())
                    info.longest_word = current;
            }
        }
        file.close();
        return info;
    }
};