#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <csignal>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

constexpr int PORT = 5555;
constexpr int BACKLOG = 5;
volatile std::sig_atomic_t server_stop = 0;
int server_socket = -1;

void server_signal_control(int)
{
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
    while(total_bytes_sent < len)
    {
        ssize_t sent = ::send(socket, data + total_bytes_sent, len - total_bytes_sent, 0);
        if(sent <= 0)
            return sent;
        total_bytes_sent += static_cast<size_t>(sent);
    }
    return static_cast<ssize_t>(total_bytes_sent);
}

bool send_line(int socket, const std::string& line)
{
    std::string data = line + "\n";
    return send_data(socket, data.c_str(), data.size()) > 0;
}

bool get_line(int socket, std::string& out)
{
    out.clear();
    char ch;
    while(1)
    {
        ssize_t n = ::recv(socket, &ch, 1, 0);
        if (n == 0)
            return 0;
        if(n < 0)
        {
            if(errno == EINTR)
                continue;
            perror("recv");
            return 0;
        }
        if(ch == '\n')
            break;
        out.push_back(ch);
        if(out.size() > 4096)
        {
            std::cerr << "Line too long, closng\n";
            return 0;
        }
    }
    return 1;
}

void client_procc(int cl_socket)
{
    std::vector<std::string> history;
    std::string line;

    while(1)
    {
        if(!get_line(cl_socket, line))
        {
            std::cout << "client disconnected\n";
            break;
        }

        if(line == "PING")
            send_line(cl_socket, "PONG");
        else if (line.rfind("MSG ", 0) == 0)
        {
            std::string msg = line.substr(4);
            history.push_back(msg);
            send_line(cl_socket, "MSG_OK");
        }
        else if(line == "HISTORY")
        {
            for(const std::string& h : history)
                send_line(cl_socket, h);
            send_line(cl_socket, "END_HIST");
        }
        else if(line == "QUIT")
        {
            send_line(cl_socket, "EXIT");
            std::cout << "client disconnected\n";
            break;
        }
        else
        {
            send_line(cl_socket, "ERROR:UNKNOWN COMMAND");
        }
    }
    ::close(cl_socket);
}

int main()
{
    std::signal(SIGINT, server_signal_control);
    server_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0)
    {
        perror("socket");
        return 1;
    }
    int option = 1;
    if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0)
    {
        perror("setsocketoption");
        ::close(server_socket);
        return 1;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);
    if(bind(server_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        perror("bind");
        ::close(server_socket);
        return 1;
    }
    if(listen(server_socket, BACKLOG) < 0)
    {
        perror("listen");
        ::close(server_socket);
        return 1;
    }
    std::cout << "Server listening on port " << PORT << "\n";

    while(!server_stop)
    {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_socket = ::accept(server_socket, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if(client_socket < 0)
        {
            if(server_stop) break;
            perror("accept");
            continue;
        }
        std::cout << "Client connected\n";
        client_procc(client_socket);
    }
    if (server_socket != -1)
    ::close(server_socket);
    std::cout << "Server stopped\n";
}