#include <iostream>
#include <string>
#include <cstring>
#include <csignal>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

constexpr int PORT = 5555;
constexpr const char* SERVER_IP = "127.0.0.1";
constexpr size_t MAX_LINE_LENGTH = 4096;

volatile std::sig_atomic_t client_stop = 0;
int client_socket = -1;

void client_signal_control(int signum)
{
    std::cout << "\nShutting down client...\n";
    client_stop = 1;
    if (client_socket != -1)
    {
        ::close(client_socket);
        client_socket = -1;
    }
}

ssize_t send_data(int socket, const char* data, size_t len)
{
    size_t total_bytes_sent = 0;
    while(total_bytes_sent < len && !client_stop)
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
    if (client_stop) return false;
    
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
    
    while(!client_stop)
    {
        ssize_t n = ::recv(socket, &ch, 1, 0);
        if (n == 0)
        {
            std::cout << "Server closed connection\n";
            return false;
        }
        if(n < 0)
        {
            if(errno == EINTR)
                continue;
            if(errno == ECONNRESET)
            {
                std::cout << "Connection reset by server\n";
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
            std::cerr << "Line too long from server\n";
            return false;
        }
    }
    return true;
}

bool connect_to_server()
{
    client_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if(client_socket < 0)
    {
        perror("socket creation failed");
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    if(inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        perror("invalid server address");
        ::close(client_socket);
        client_socket = -1;
        return false;
    }

    std::cout << "Connecting to server " << SERVER_IP << ":" << PORT << std::endl;
    
    if(connect(client_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0)
    {
        perror("connect failed");
        ::close(client_socket);
        client_socket = -1;
        return false;
    }

    std::cout << "Connected to server successfully\n";
    return true;
}

void test_basic_commands()
{
    std::string line;
    
    // Read welcome message
    if (get_line(client_socket, line))
    {
        std::cout << "Server: " << line << std::endl;
    }

    // Test PING-PONG
    std::cout << "\nTesting PING-PONG..." << std::endl;
    if (send_line(client_socket, "PING"))
    {
        if (get_line(client_socket, line))
        {
            std::cout << "Server: " << line << std::endl;
            if (line == "PONG")
            {
                std::cout << "PING-PONG test: SUCCESS\n";
            }
            else
            {
                std::cout << "PING-PONG test: FAILED\n";
            }
        }
    }

    // Test message sending
    std::cout << "\nTesting message sending..." << std::endl;
    if (send_line(client_socket, "MSG Hello from client!"))
    {
        if (get_line(client_socket, line))
        {
            std::cout << "Server: " << line << std::endl;
            if (line == "MSG_OK")
            {
                std::cout << "Message test: SUCCESS\n";
            }
        }
    }

    // Test unknown command
    std::cout << "\nTesting error handling..." << std::endl;
    if (send_line(client_socket, "UNKNOWN_COMMAND"))
    {
        if (get_line(client_socket, line))
        {
            std::cout << "Server: " << line << std::endl;
        }
    }
}

void interactive_mode()
{
    std::string input;
    std::string response;
    
    std::cout << "\n=== Interactive Mode ===" << std::endl;
    std::cout << "Available commands:" << std::endl;
    std::cout << "  PING           - Test connection" << std::endl;
    std::cout << "  MSG <message>  - Send a message" << std::endl;
    std::cout << "  HISTORY        - Get message history" << std::endl;
    std::cout << "  STATUS         - Get server status" << std::endl;
    std::cout << "  QUIT           - Disconnect from server" << std::endl;
    std::cout << "=================================\n" << std::endl;

    while(!client_stop)
    {
        std::cout << "Enter command: ";
        if (!std::getline(std::cin, input) || client_stop)
            break;

        if (input.empty())
            continue;

        if (!send_line(client_socket, input))
        {
            std::cerr << "Failed to send command\n";
            break;
        }

        if (input == "QUIT")
        {
            // Read goodbye message
            if (get_line(client_socket, response))
            {
                std::cout << "Server: " << response << std::endl;
            }
            break;
        }

        // Read server response
        while (get_line(client_socket, response))
        {
            std::cout << "Server: " << response << std::endl;
            if (response == "END_HIST" || response.rfind("ERROR:", 0) == 0 || 
                response.rfind("SERVER:", 0) == 0 || response == "MSG_OK" || 
                response == "PONG")
            {
                break;
            }
        }

        if (client_socket == -1)
            break;
    }
}

int main()
{
    std::signal(SIGINT, client_signal_control);
    std::signal(SIGTERM, client_signal_control);

    if (!connect_to_server())
    {
        return 1;
    }

    // Run basic tests
    test_basic_commands();

    // Start interactive mode
    interactive_mode();

    if (client_socket != -1)
    {
        ::close(client_socket);
        client_socket = -1;
    }

    std::cout << "Client stopped\n";
    return 0;
}
