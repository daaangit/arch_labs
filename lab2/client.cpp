#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

constexpr int PORT = 5555;

ssize_t send_data(int socket, const char* data, size_t len)
{
    size_t total_bytes_sent = 0;
    while (total_bytes_sent < len)
    {
        ssize_t sent = ::send(socket, data + total_bytes_sent, len - total_bytes_sent, 0);
        if (sent <= 0)
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
    while (1)
    {
        ssize_t n = ::recv(socket, &ch, 1, 0);
        if (n == 0)
            return 0;
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            perror("recv");
            return 0;
        }
        if (ch == '\n')
            break;
        out.push_back(ch);
        if (out.size() > 4096)
        {
            std::cerr << "Line too long\n";
            return 0;
        }
    }
    return 1;
}

int main()
{
    int client_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (::inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        ::close(client_socket);
        return 1;
    }

    if (::connect(client_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0)
    {
        perror("connect");
        ::close(client_socket);
        return 1;
    }

    std::cout << "Connected to server on 127.0.0.1:" << PORT << "\n";
    std::cout << "Commands:\n";
    std::cout << "  ping              - send PING\n";
    std::cout << "  msg <text>        - send message to history\n";
    std::cout << "  history           - get message history\n";
    std::cout << "  quit              - send QUIT and exit\n";

    std::string input;
    std::string line;

    while (1)
    {
        std::cout << "> ";
        if (!std::getline(std::cin, input))
            break;

        if (input == "ping")
        {
            if (!send_line(client_socket, "PING"))
            {
                std::cerr << "send error\n";
                break;
            }
            if (!get_line(client_socket, line))
                break;
            std::cout << "< " << line << "\n";
        }
        else if (input.rfind("msg ", 0) == 0)
        {
            std::string msg = input.substr(4);
            if (!send_line(client_socket, "MSG " + msg))
            {
                std::cerr << "send error\n";
                break;
            }
            if (!get_line(client_socket, line))
                break;
            std::cout << "< " << line << "\n";
        }
        else if (input == "history")
        {
            if (!send_line(client_socket, "HISTORY"))
            {
                std::cerr << "send error\n";
                break;
            }

            while (1)
            {
                if (!get_line(client_socket, line))
                    goto end;
                if (line == "END_HIST")
                    break;
                std::cout << "  " << line << "\n";
            }
        }
        else if (input == "quit")
        {
            if (!send_line(client_socket, "QUIT"))
            {
                std::cerr << "send error\n";
                break;
            }
            if (get_line(client_socket, line))
                std::cout << "< " << line << "\n";
            break;
        }
        else if (input.empty())
        {
            continue;
        }
        else
        {
            std::cout << "Unknown command\n";
        }
    }

end:
    ::close(client_socket);
    std::cout << "Disconnected\n";
    return 0;
}
