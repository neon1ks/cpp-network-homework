#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <thread>


#ifdef _WIN32
#   define ioctl ioctlsocket
#else
extern "C"
{
#   include <netinet/tcp.h>
#   include <sys/ioctl.h>
}
#endif


#include <socket_wrapper/socket_class.h>
#include <socket_wrapper/socket_headers.h>
#include <socket_wrapper/socket_wrapper.h>


namespace
{

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


std::string read_host()
{
    std::cout << "Put host: ";
    std::string host;
    std::cin >> host;
    return host;
}


uint16_t read_port()
{
    std::cout << "Put port: ";
    uint16_t port{};
    std::cin >> port;
    while (std::cin.get() != '\n')
        ; // do nothing
    return port;
}



bool send_request(socket_wrapper::Socket &sock, const std::string &request)
{
    ssize_t bytes_count = 0;
    size_t req_pos = 0;
    const auto *const req_buffer = &(request.c_str()[0]);
    auto const req_length = request.length();

    while (true)
    {
        if ((bytes_count = send(sock, req_buffer + req_pos, req_length - req_pos, 0)) < 0)
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


void recv_answer(socket_wrapper::Socket &sock, std::vector<char> &buffer)
{
    while (true)
    {
        auto recv_bytes = recv(sock, buffer.data(), buffer.size() - 1, 0);

        std::cout
            << recv_bytes
            << " was received..."
            << std::endl;

        if (recv_bytes > 0)
        {
            buffer[static_cast<size_t>(recv_bytes)] = '\0';
            std::cout << "------------\n" << std::string(buffer.begin(), std::next(buffer.begin(), recv_bytes)) << std::endl;
            continue;
        }

        if (-1 == recv_bytes)
        {
            if (EINTR == errno) continue;
            if (0 == errno) break;
            // std::cerr << errno << ": " << sock_wrap.get_last_error_string() << std::endl;
            break;
        }

        break;
    }
}

} // namespace


int main()
{
    const socket_wrapper::SocketWrapper sock_wrap;

    const std::string host = read_host();
    const uint16_t    port = read_port();

    socket_wrapper::Socket sock = { 0 };

    addrinfo hints = { .ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM };

    addrinfo* serv_info = nullptr;

    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &serv_info) < 0)
    {
        std::cerr << "Cannot resolve address: "
                  << host
                  << " and port: "
                  << port
                  << std::endl;

        return EXIT_FAILURE;
    }

    for (auto ai = serv_info; ai != nullptr; ai = ai->ai_next)
    {
        sock = {ai->ai_family, ai->ai_socktype, ai->ai_protocol};

        if (!sock)
        {
            std::cerr << sock_wrap.get_last_error_string() << std::endl;
            return EXIT_FAILURE;
        }

        if (connect(sock, ai->ai_addr, ai->ai_addrlen) != 0)
        {
            std::cerr << sock_wrap.get_last_error_string() << std::endl;
            return EXIT_FAILURE;
        }
        else
        {
            std::cout << "Connected to \"" << host << "\"..." << std::endl;
            break;
        }
    }

    std::vector<char> buffer;
    buffer.resize(Max_Message_Length);

    const IoctlType flag = 1;

           // Put the socket in non-blocking mode:
    if (ioctl(sock, FIONBIO, const_cast<IoctlType*>(&flag)) < 0)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

           // Disable Naggles's algorithm.
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char *>(&flag), sizeof(flag)) < 0)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    while (true)
    {
        std::string text;
        std::cout << "Write text to send (or 'exit'): ";
        std::getline(std::cin, text);
        trim(text);

        if (text.size() >= Max_Message_Length)
        {
            text.resize(Max_Message_Length);
            text[Max_Message_Length - 3] = '\r';
            text[Max_Message_Length - 2] = '\n';
            text[Max_Message_Length - 1] = '\0';
        }

        if (!send_request(sock, text))
        {
            std::cerr << sock_wrap.get_last_error_string() << std::endl;
            return EXIT_FAILURE;
        }

        std::cout
            << "Request was sent, reading response..."
            << std::endl;

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(2ms);

        recv_answer(sock, buffer);
    }

    return EXIT_SUCCESS;
}
