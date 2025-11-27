cmake_minimum_required(VERSION 3.10)
project(lab2_chat)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Компиляция сервера
add_executable(chat_server server.cpp)
target_compile_options(chat_server PRIVATE -Wall -Wextra -Werror)

# Компиляция клиента  
add_executable(chat_client client.cpp)
target_compile_options(chat_client PRIVATE -Wall -Wextra -Werror)

# Установка флагов для Linux
if(UNIX)
    target_link_libraries(chat_server pthread)
    target_link_libraries(chat_client pthread)
endif()
