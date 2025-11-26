#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <csignal>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

constexpr int PORT = 5555;
constexpr int BACKLOG = 5;
constexpr size_t MAX_LINE_LENGTH = 4096;
constexpr size_t MAX_HISTORY_SIZE = 1000;

volatile std::sig_atomic_t server_stop = 0;
int server_socket = -1;

void server_signal_control(int signum)
{
    std::cout << "\nReceived signal " << signum << ", shutting down server...\n";
    server_stop = 1;
    if (server_socket != -1)
    {
        ::close(server_socket);
        server_socket = -1;
    }
}

ssize_t send_data(int socket, const char* data, size_t len)
{
    size_t total_bytes_sent = 0;
    while(total_bytes_sent < len && !server_stop)
    {
        ssize_t sent = ::send(socket, data + total_bytes_sent, len - total_bytes_sent, MSG_NOSIGNAL);
        if(sent < 0)
        {
            if(errno == EINTR)
                continue;
            perror("send failed");
            return -1;
        }
        if(sent == 0)
            return 0;
        total_bytes_sent += static_cast<size_t>(sent);
    }
    return static_cast<ssize_t>(total_bytes_sent);
}

bool send_line(int socket, const std::string& line)
{
    if (server_stop) return false;
    
    std::string data = line + "\n";
    ssize_t result = send_data(socket, data.c_str(), data.size());
    if (result <= 0)
    {
        std::cerr << "Failed to send line: " << line << std::endl;
        return false;
    }
    return true;
}

bool get_line(int socket, std::string& out)
{
    out.clear();
    char ch;
    
    while(!server_stop)
    {
        ssize_t n = ::recv(socket, &ch, 1, 0);
        if (n == 0)
        {
            std::cout << "Client closed connection\n";
            return false;
        }
        if(n < 0)
        {
            if(errno == EINTR)
                continue;
            if(errno == ECONNRESET)
            {
                std::cout << "Connection reset by client\n";
                return false;
            }
            perror("recv failed");
            return false;
        }
        
        if(ch == '\n')
            break;
            
        out.push_back(ch);
        if(out.size() > MAX_LINE_LENGTH)
        {
            std::cerr << "Line too long, closing connection\n";
            send_line(socket, "ERROR:LINE_TOO_LONG");
            return false;
        }
    }
    return true;
}

void client_session(int client_socket)
{
    std::vector<std::string> history;
    std::string line;
    bool authenticated = false;

    // Send welcome message
    if (!send_line(client_socket, "SERVER:Welcome to Chat Server"))
    {
        ::close(client_socket);
        return;
    }

    while(!server_stop)
    {
        if(!get_line(client_socket, line))
            break;

        std::cout << "Received from client: " << line << std::endl;

        if(line == "PING")
        {
            if (!send_line(client_socket, "PONG"))
                break;
        }
        else if (line.rfind("MSG ", 0) == 0)
        {
            if (line.length() <= 4)
            {
                send_line(client_socket, "ERROR:EMPTY_MESSAGE");
                continue;
            }
            
            std::string msg = line.substr(4);
            // Validate message length
            if (msg.length() > 1000)
            {
                send_line(client_socket, "ERROR:MESSAGE_TOO_LONG");
                continue;
            }
            
            history.push_back(msg);
            if (history.size() > MAX_HISTORY_SIZE)
                history.erase(history.begin());
                
            if (!send_line(client_socket, "MSG_OK"))
                break;
        }
        else if(line == "HISTORY")
        {
            if (history.empty())
            {
                if (!send_line(client_socket, "SERVER:No history available"))
                    break;
            }
            else
            {
                for(const std::string& h : history)
                {
                    if (!send_line(client_socket, h))
                        break;
                }
            }
            if (!send_line(client_socket, "END_HIST"))
                break;
        }
        else if(line == "QUIT")
        {
            send_line(client_socket, "SERVER:Goodbye");
            std::cout << "Client requested disconnect\n";
            break;
        }
        else if(line == "STATUS")
        {
            std::string status = "SERVER:Status - Messages in history: " + 
                                std::to_string(history.size()) +
                                ", Connected clients: 1";
            if (!send_line(client_socket, status))
                break;
        }
        else
        {
            if (!send_line(client_socket, "ERROR:UNKNOWN_COMMAND"))
                break;
        }
    }
    
    ::close(client_socket);
    std::cout << "Client session ended\n";
}

int main()
{
    std::signal(SIGINT, server_signal_control);
    std::signal(SIGTERM, server_signal_control);

    server_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0)
    {
        perror("socket creation failed");
        return 1;
    }

    int option = 1;
    if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0)
    {
        perror("setsockopt failed");
        ::close(server_socket);
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);
    
    if(bind(server_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        perror("bind failed");
        ::close(server_socket);
        return 1;
    }
    
    if(listen(server_socket, BACKLOG) < 0)
    {
        perror("listen failed");
        ::close(server_socket);
        return 1;
    }
    
    std::cout << "Server listening on port " << PORT << std::endl;
    std::cout << "Press Ctrl+C to stop the server\n";

    while(!server_stop)
    {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_socket = ::accept(server_socket, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        
        if(client_socket < 0)
        {
            if(server_stop) break;
            if(errno == EINTR) continue;
            perror("accept failed");
            continue;
        }

        std::cout << "New client connected\n";
        
        client_session(client_socket);
    }

    if (server_socket != -1)
    {
        ::close(server_socket);
        server_socket = -1;
    }
    
    std::cout << "Server stopped gracefully\n";
    return 0;
}
