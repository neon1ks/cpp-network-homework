#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include <cstring>
#include <socket_wrapper/socket_class.h>
#include <socket_wrapper/socket_headers.h>
#include <socket_wrapper/socket_wrapper.h>


namespace
{

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

} // namespace


int main()
{
    const socket_wrapper::SocketWrapper sock_wrap;

    const std::string host = read_host();
    const uint16_t    port = read_port();

    const socket_wrapper::Socket sock = { AF_INET, SOCK_DGRAM, IPPROTO_UDP };

    if (!sock)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    sockaddr_in addr = {
        .sin_family = PF_INET,                    //
        .sin_port   = htons(port),                //
        .sin_addr   = { inet_addr(host.c_str()) } //
    };

    if (connect(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        // Socket will be closed in the Socket destructor.
        return EXIT_FAILURE;
    }

    auto buffer = std::make_unique<char*>(new char[MaxMessageLength]);

    while (true)
    {
        std::string text;
        std::cout << "Write text to send (or 'exit'): ";
        std::getline(std::cin, text);
        trim(text);

        if (text.size() >= MaxMessageLength)
        {
            text.resize(MaxMessageLength);
            text[MaxMessageLength - 1] = '\0';
        }

        sendto(sock, text.c_str(), text.length(), 0, nullptr, sizeof(addr));

        auto len = recvfrom(sock, *buffer, MaxMessageLength - 1, 0, nullptr, nullptr);

        if (len > 0)
        {
            const std::string answer = { *buffer, static_cast<std::size_t>(len) };
            std::cout << "Answer from server: " << answer << std::endl;

            if (answer == "exit")
            {
                break;
            }
        }
    }

    return EXIT_SUCCESS;
}
