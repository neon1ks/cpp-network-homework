#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include <socket_wrapper/socket_headers.h>
#include <socket_wrapper/socket_wrapper.h>
#include <socket_wrapper/socket_class.h>


namespace {

bool exit_state = false;

constexpr std::size_t Max_Message_Length = 1000;
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


bool send_message(socket_wrapper::Socket& sock, const std::string& message)
{
    size_t            req_pos    = 0;
    const auto* const req_buffer = &(message.c_str()[0]);
    auto const        req_length = message.length();

    while (true)
    {
        const ssize_t bytes_count = send(sock, req_buffer + req_pos, req_length - req_pos, 0);
        if (bytes_count < 0)
        {
            if (EINTR == errno)
            {
                continue;
            }
        }
        else
        {
            if (bytes_count == 0)
            {
                break;
            }

            req_pos += static_cast<size_t>(bytes_count);

            if (req_pos >= req_length)
            {
                break;
            }
        }
    }

    return true;
}


bool communicate(socket_wrapper::Socket& sock, std::vector<char> &buffer)
{
    std::mutex mu;
    while (true)
    {
        auto recv_bytes = recv(sock, buffer.data(), buffer.size() - 1, 0);

        mu.lock();
        std::cout << recv_bytes << " was received..." << std::endl;
        mu.unlock();

        if (recv_bytes > 0)
        {
            buffer[static_cast<size_t>(recv_bytes)] = '\0';

            mu.lock();
            std::cout << "\n"
                      << std::string(buffer.begin(),
                                     std::next(buffer.begin(), recv_bytes))
                      << std::endl;
            mu.unlock();

            send_message(sock, buffer.data());

            continue;
        }

        if (-1 == recv_bytes)
        {
            if (EINTR == errno)
            {
                continue;
            }
            if (0 == errno)
            {
                break;
            }
            break;
        }

        break;
    }
    return true;
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
    const socket_wrapper::Socket sock = { AF_INET, SOCK_STREAM, IPPROTO_TCP };

    const int port { std::stoi(argv[1]) };
    std::cout << "Starting Echo TCP server on the port" << port << "...\n";

    if (!sock)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(static_cast<uint16_t>(port)),
    };

    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        // Socket will be closed in the Socket destructor.
        return EXIT_FAILURE;
    }

    const int queue = 5;

    listen(sock, queue);

    std::vector<char> buffer;
    buffer.resize(Max_Message_Length);

    // socket address used to store client address
    struct sockaddr_storage client_address     = { 0 };
    socklen_t               client_address_len = sizeof(client_address);

    std::cout << "Running TCP server...\n" << std::endl;

    // new socket to accept
    socket_wrapper::Socket   newsock = { 0 };

    while (!exit_state)
    {
        // accepting client
        newsock = accept(sock, reinterpret_cast<sockaddr*>(&client_address), &client_address_len);

        if (!newsock)
        {
            std::cerr << "ERROR on accept!" << std::endl;
            return EXIT_FAILURE;
        }

        exit_state = communicate(newsock, buffer);
    }

    return EXIT_SUCCESS;
}

