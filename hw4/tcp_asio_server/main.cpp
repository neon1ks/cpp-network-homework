#include <boost/asio/ts/buffer.hpp>
#include <boost/asio/ts/internet.hpp>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <utility>

using boost::asio::ip::tcp;


class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(tcp::socket socket)
        : socket_(std::move(socket))
    {
    }


    void start()
    {
        std::cout << "Client accepted\n";
        do_read();
    }

private:
    tcp::socket socket_;
    enum
    {
        max_length = 1024
    };
    char data_[max_length + 1];


    void do_read()
    {
        auto self(shared_from_this());
        // clang-format off
        socket_.async_read_some(
            boost::asio::buffer(data_, max_length),
            [this, self](std::error_code ec, std::size_t length)
                {
                    if (!ec)
                    {
                        do_command(length);
                    }
                });
        // clang-format on
    }


    void do_command(std::size_t length)
    {
        auto self(shared_from_this());
        // Command analyse
        data_[length] = '\0';
        const std::string dat(data_);
        std::cout << dat << std::endl;
        std::istringstream iss(dat);
        std::string        command   = { 0 };
        std::string        filename  = { 0 };
        std::string        fiction   = { 0 };
        size_t             fict      = 0;
        int                size_fict = 0;

        iss >> command >> filename >> fiction >> size_fict;
        if ("exit" == command)
        {
            std::cout << "Breaking" << std::endl;
            std::exit(0);
        }

        // clang-format off
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(data_, length),
            [this, self](std::error_code ec, std::size_t /*length*/)
                {
                    if (!ec)
                    {
                        do_read();
                    }
                }
        );
        // clang-format on
    }
};


class Server
{
public:
    Server(boost::asio::io_context& ioc, short port)
        : acceptor_(ioc, tcp::endpoint(tcp::v4(), port))
        , socket_(ioc)
    {
        do_accept();
    }


private:
    void do_accept()
    {
        // clang-format off
        acceptor_.async_accept(
            socket_,
            [this](std::error_code ec)
                {
                    if (!ec)
                    {
                        std::make_shared<Session>(std::move(socket_))->start();
                    }
                    do_accept();
                }
        );
        // clang-format on
    }


    tcp::acceptor acceptor_;
    tcp::socket   socket_;
};

int main(int argc, char* argv[])
{
    std::cout << "Running echo server...\n" << std::endl;
    std::cout << "Command:\n" << std::endl;
    std::cout << "        exit - to quit\n" << std::endl;

    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: " << argv[0] << " <port>\n";
            return 1;
        }

        boost::asio::io_context ioc;
        const Server server(ioc, static_cast<std::int16_t>(std::atoi(argv[1])));
        ioc.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "Unknow error\n";
    }

    return 0;
}
