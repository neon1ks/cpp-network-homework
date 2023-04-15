#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>

#include <socket_wrapper/socket_class.h>
#include <socket_wrapper/socket_headers.h>
#include <socket_wrapper/socket_wrapper.h>

#include <netdb.h>
#include <sys/socket.h>

#include <filesystem>
#include <fstream>
#include <optional>

namespace fs = std::filesystem;
// const auto buffer_size = 4096;

#if !defined(MAX_PATH)
#define MAX_PATH (256)
#endif

const wchar_t separ = *reinterpret_cast<const wchar_t*>(&fs::path::preferred_separator);

class Transceiver
{
public:
    Transceiver(int client_sock, int fict, size_t size_fict)
        : client_sock_(client_sock)
        , fict_(fict)
        , size_fict_(size_fict){};
    Transceiver(const Transceiver&) = delete;
    Transceiver()                   = delete;

public:
    [[nodiscard]] int ts_socket() const { return client_sock_; }


public:
    bool send_buffer(const std::vector<char>& buffer) const
    {
        size_t     transmit_bytes_count = 0;
        const auto size                 = buffer.size();

        while (transmit_bytes_count != size)
        {
            auto result =
                send(client_sock_, &(buffer.data()[0]) + transmit_bytes_count, size - transmit_bytes_count, 0);
            if (-1 == result)
            {
                if (need_to_repeat())
                    continue;
                return false;
            }

            transmit_bytes_count += result;
        }

        return true;
    }


    bool send_file(fs::path const& file_path)
    {

        std::ifstream file_stream(file_path, std::ifstream::binary);

        if (!file_stream)
            return false;

        std::cout << "Sending file " << file_path << "..." << std::endl;

        const size_t      buffer_size = 4096;
        std::vector<char> buffer(buffer_size);

        file_stream.seekg(0, std::ios::end);
        size_t count = file_stream.tellg();
        file_stream.seekg(0, std::ios::beg);

        if (size_fict_ > count)
            size_fict_ = count;

        if (1 == fict_)
        {
            file_stream.seekg(size_fict_);
        }
        else if (2 == fict_)
        {
            count = size_fict_;
        }

        while (file_stream && count > 0)
        {
            if (count < buffer_size)
            {
                file_stream.read(&buffer[0], count);
                count = 0;
            }
            else
            {
                file_stream.read(&buffer[0], buffer.size());
                count -= buffer_size;
            }

            if (!send_buffer(buffer))
                return false;
        }

        return true;
    }


private:
    static bool need_to_repeat()
    {
        switch (errno)
        {
            case EINTR:
            case EAGAIN:
                // case EWOULDBLOCK: // EWOULDBLOCK == EINTR.
                return true;
        }

        return false;
    };

    int fict_;
    int client_sock_;

    size_t size_fict_;
};

class Client
{
private:
    Transceiver tsr_;
    fs::path    file_path_;
    std::string filename_;


public:
    Client(int sock, std::string filename, int fict, size_t size_fict)
        : tsr_(sock, fict, size_fict)
        , filename_(filename)
    {
        std::cout << "Client [" << static_cast<int>(tsr_.ts_socket()) << "] "
                  << "was created..." << std::endl;
    }


    std::optional<fs::path> recv_file_path()
    {
        auto request_data = filename_;
        if (request_data.empty())
            return std::nullopt;

        auto cur_path  = fs::current_path().wstring();
        auto file_path = fs::weakly_canonical(request_data).wstring();

        if (file_path.find(cur_path) == 0)
        {
            file_path = file_path.substr(cur_path.length());
        }

        return fs::weakly_canonical(cur_path + separ + file_path);
    }


    bool send_file(const fs::path& file_path)
    {
        if (!(fs::exists(file_path) && fs::is_regular_file(file_path)))
            return false;

        return tsr_.send_file(file_path);
    }


    bool process()
    {
        auto file_to_send = recv_file_path();
        bool result       = false;

        if (std::nullopt != file_to_send)
        {
            std::cout << "Trying to send " << *file_to_send << "..." << std::endl;
            if (send_file(*file_to_send))
            {
                std::cout << "File was sent." << std::endl;
            }
            else
            {
                std::cerr << "File sending error!" << std::endl;
            }
            result = true;
        }

        return result;
    }
};


int main(int argc, char const* argv[])
{

    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
        return EXIT_FAILURE;
    }

    // socket_wrapper::SocketWrapper sock_wrap;
    const int port{ std::stoi(argv[1]) };

    struct addrinfo hints, *addr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;
    getaddrinfo(NULL, argv[1], &hints, &addr);

    int sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

    std::cout << "Starting echo server on the port " << port << "...\n";

    if (!sock)
    {
        //    std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    char yes = '1';
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)))
    {
        //    std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    if (bind(sock, addr->ai_addr, addr->ai_addrlen) != 0)
    {
        // std::cerr << sock_wrap.get_last_error_string() << std::endl;
        // Socket will be closed in the Socket destructor.
        return EXIT_FAILURE;
    }

    // socket address used to store client address
    ssize_t recv_len = 0;

    std::cout << "Running echo server...\n" << std::endl;
    std::cout << "Command:\n" << std::endl;
    std::cout << "        exit - to quit\n" << std::endl;
    std::cout << "        get <filename> <beans/size> <number>\n" << std::endl;

    bool run = true;

    listen(sock, 5);

    struct sockaddr_storage client_addr = { 0 };
    socklen_t               client_len  = sizeof(client_addr);
    char                    ipbuff[INET_ADDRSTRLEN];

    int newsockfd = accept(sock, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    shutdown(sock, SHUT_RDWR);
    std::string str_buffer;

    while (run)
    {
        if (int(newsockfd) < 0)
        {
            throw std::runtime_error("ERROR on accept");
        }
        char buffer[256] = { 0 };
        recv_len         = recv(newsockfd, buffer, sizeof(buffer) - 1, 0);
        if (recv_len > 0)
        {
            // buffer[recv_len] = '\0';
            str_buffer = std::string(buffer);

            char hbuf[NI_MAXHOST] = "\0";
            getnameinfo(
                reinterpret_cast<sockaddr*>(&client_addr), client_len, hbuf, sizeof(hbuf), nullptr, 0, NI_NAMEREQD);

            if (AF_INET == ((struct sockaddr_in*)&client_addr)->sin_family)
            {
                inet_ntop(
                    client_addr.ss_family, &(((struct sockaddr_in*)&client_addr)->sin_addr), ipbuff, INET_ADDRSTRLEN);
            }
            else if (AF_INET6 == ((struct sockaddr_in6*)&client_addr)->sin6_family)
            {
                inet_ntop(
                    client_addr.ss_family, &(((struct sockaddr_in6*)&client_addr)->sin6_addr), ipbuff, INET_ADDRSTRLEN);
            }
            else
            {
                *ipbuff = client_addr.ss_family;
            }
            std::cout << "Client with address " << ipbuff << "(name=" << hbuf << ")"
                      << ":" << ((struct sockaddr_in*)&client_addr)->sin_port << " sent datagram "
                      << "[length = " << str_buffer.length() << "]:\n'''\n"
                      << str_buffer << "\n'''" << std::endl;

            send(newsockfd, str_buffer.c_str(), str_buffer.length(), 0);

            // Command analyse
            std::istringstream iss(str_buffer);
            std::string        command   = { 0 };
            std::string        filename  = { 0 };
            std::string        fiction   = { 0 };
            size_t             fict      = 0;
            int                size_fict = 0;

            iss >> command >> filename >> fiction >> size_fict;
            if ("exit" == command)
            {
                std::cout << "Breaking" << std::endl;
                run = false;
            }

            if ("get" == command && str_buffer.length() > 4)
            {
                if ("beans" == fiction)
                {
                    fict = 1;
                }
                else if ("size" == fiction)
                {
                    fict = 2;
                }

                Client client(newsockfd, filename, fict, size_fict);
                auto   result = client.process();

                if (result)
                    std::cout << "The end" << std::endl;
            }
        }
    }

    shutdown(newsockfd, SHUT_RDWR);

    return EXIT_SUCCESS;
}
