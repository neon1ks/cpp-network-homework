#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

// #define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"


int main(int argc, const char* const argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <http_server> <port>" << std::endl;
        return EXIT_FAILURE;
    }
    // HTTP
    httplib::Client cli(argv[1], std::stoi(argv[2]));

    auto res1 = cli.Get("/");
    std::cout << res1->status << "\n"
              << "Status:" << res1->body << std::endl
              << std::endl;

    size_t lenght_f = 0;

    char* body = new char[10000];
    char* p    = body;

    std::string filename = "hello";

    httplib::Params params = {
        { "filename", filename },
        // { "size", "1000" }
        // { "beans", "1000" }
    };

    httplib::Headers headers = {};

    // clang-format off
    auto res2 = cli.Get(
        "/get",
        params,
        headers,
        [&](const char* data, size_t data_length)
        {
            lenght_f = lenght_f + data_length;
            for (size_t i = 0; i < data_length; ++i)
            {
                *p = *data;
                ++p;
                ++data;
            }
            return true;
        }
    );
    // clang-format on

    std::ofstream ofs(filename, std::ios::binary);
    body[lenght_f] = '\0';
    ofs.write(body, lenght_f);
    ofs.close();

    std::cout << "Receive file ...\n"
              << "Status:" << res2->status << "\n"
              << body << std::endl
              << std::endl;
    delete[] body;

    auto res3 = cli.Get("/exit");
    std::cout << "Status:" << res3->status << "\n" << res3->body << std::endl;
}
