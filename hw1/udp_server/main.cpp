#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include <socket_wrapper/socket_headers.h>
#include <socket_wrapper/socket_wrapper.h>
#include <socket_wrapper/socket_class.h>


namespace {

constexpr std::size_t MaxMessageLength = 1000;
const char* ws = " \t\n\r\f\v";


// trim from end of string (right)
inline std::string& rtrim(std::string& s, const char* t = ws)
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}


// trim from beginning of string (left)
inline std::string& ltrim(std::string& s, const char* t = ws)
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}


// trim from both ends of string (right then left)
inline std::string& trim(std::string& s, const char* t = ws)
{
    return ltrim(rtrim(s, t), t);
}


std::string host_by_ip(char *addr)
{
    struct in_addr in;
    if (inet_aton(addr, &in) != 0)
    {
        auto *hp = gethostbyaddr(reinterpret_cast<char*>(&in.s_addr), sizeof(in.s_addr), AF_INET);
        if (hp != nullptr)
        {
            return hp->h_name;
        }
    }

    return "Unknown";
}

} // namespace


int main(int argc, char const *argv[])
{

    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
        return EXIT_FAILURE;
    }

    const socket_wrapper::SocketWrapper sock_wrap;
    const socket_wrapper::Socket sock = {AF_INET, SOCK_DGRAM, IPPROTO_UDP};

    const int port { std::stoi(argv[1]) };
    std::cout << "Starting echo server on the port " << port << "...\n";

    if (!sock)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    sockaddr_in addr = {
        .sin_family = PF_INET,
        .sin_port   = htons(static_cast<uint16_t>(port)),
    };

    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        // Socket will be closed in the Socket destructor.
        return EXIT_FAILURE;
    }

    char buffer[MaxMessageLength];

    // socket address used to store client address
    struct sockaddr_in client_address = {0};
    socklen_t client_address_len = sizeof(sockaddr_in);
    ssize_t recv_len = 0;

    std::cout << "Running echo server...\n" << std::endl;
    char client_address_buf[INET_ADDRSTRLEN];

    bool run = true;
    while (run)
    {
        // Read content into buffer from an incoming client.
        recv_len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                            reinterpret_cast<sockaddr *>(&client_address),
                            &client_address_len);

        std::string command_string = {reinterpret_cast<const char*>(buffer), static_cast<std::size_t>(recv_len)};
        trim(command_string);

        if (recv_len > 0)
        {
            std::cout
                << "Client with address "
                << inet_ntop(AF_INET, &client_address.sin_addr, client_address_buf, sizeof(client_address_buf) / sizeof(client_address_buf[0]))
                << ":" << ntohs(client_address.sin_port)
                << " and name '"
                << host_by_ip(client_address_buf)
                << "' sent datagram "
                << "[length = "
                << recv_len
                << "]:\n    "
                << command_string
                << std::endl;

            if ("exit" == command_string) {
                run = false;
            }

            // Send same content back to the client ("echo").
            sendto(sock,
                   command_string.c_str(),
                   command_string.size(),
                   0,
                   reinterpret_cast<const sockaddr*>(&client_address),
                   client_address_len);
        }

        std::cout << std::endl;
    }

    return EXIT_SUCCESS;
}

