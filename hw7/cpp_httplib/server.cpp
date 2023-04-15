#include <iomanip>

#include <iostream>

#include <filesystem>
#include <fstream>
#include <string>

#include <optional>

// #define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

namespace fs = std::filesystem;

const size_t DATA_CHUNK_SIZE = 4;

const wchar_t separ = *reinterpret_cast<const wchar_t*>(&fs::path::preferred_separator);


std::string dump_headers(const httplib::Headers& headers)
{
    std::string s;
    char        buf[BUFSIZ];

    for (auto it = headers.begin(); it != headers.end(); ++it)
    {
        const auto& x = *it;
        snprintf(buf, sizeof(buf), "%s: %s\n", x.first.c_str(), x.second.c_str());
        s += buf;
    }

    return s;
}


std::optional<fs::path> recv_file_path(std::string& filename_)
{
    auto request_data = filename_;
    if (!request_data.size())
        return std::nullopt;

    auto cur_path  = fs::current_path().wstring();
    auto file_path = fs::weakly_canonical(request_data).wstring();

    if (file_path.find(cur_path) == 0)
    {
        file_path = file_path.substr(cur_path.length());
    }

    return fs::weakly_canonical(cur_path + separ + file_path);
}


int main(int argc, const char* const argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return EXIT_FAILURE;
    }

    // HTTP
    httplib::Server svr;
    // HTTPS
    // httplib::SSLServer svr("./sslserver.pem", "./sslserver.key");

    svr.Get("/",
            [](const httplib::Request&, httplib::Response& res) { res.set_content("Hello World!", "text/plain"); });

    svr.Get("/exit",
            [&](const httplib::Request&, httplib::Response& res)
            {
                res.set_content("exit", "text/plain");
                svr.stop();
            });

    // curl -X POST --data-binary "@hello" localhost:1234/get
    // http://news.shamcode.ru/blog/yhirose--cpp-httplib/
    svr.Get("/get",
            [&](const httplib::Request& req, httplib::Response& res)
            {
                std::string filename  = { 0 };
                size_t      fict      = 0;
                size_t      size_fict = 0;

                for (auto param : req.params)
                {
                    if (param.first == "filename")
                        filename = param.second;
                    if (param.first == "beans")
                    {
                        fict      = 1;
                        size_fict = std::stoi(param.second);
                    }
                    if (param.first == "size")
                    {
                        fict      = 2;
                        size_fict = std::stoi(param.second);
                    }
                }

                auto file_path = recv_file_path(filename);
                if (std::nullopt != file_path)
                {
                    std::cout << "Trying to send " << *file_path << "..." << std::endl;
                }
                else
                {
                    res.status = 404;
                    return false;
                }

                if (!(fs::exists(*file_path) && fs::is_regular_file(*file_path)))
                {
                    res.status = 404;
                    return false;
                }

                std::ifstream t_pc_file(*file_path, std::ifstream::binary);

                t_pc_file.seekg(0, std::ios::end);
                size_t lenght_f = t_pc_file.tellg();
                t_pc_file.seekg(0, std::ios::beg);

                if (0 == lenght_f)
                {
                    res.status = 404;
                    return false;
                }

                char* data = new char[lenght_f];

                if (size_fict > lenght_f)
                    size_fict = lenght_f;

                if (1 == fict) // beans
                {
                    t_pc_file.seekg(size_fict);
                    lenght_f = lenght_f - size_fict;
                }
                else if (2 == fict) // size
                {
                    lenght_f = size_fict;
                }

                t_pc_file.read(data, lenght_f);
                data[lenght_f] = '\0';

                res.set_content_provider(
                    lenght_f,
                    "application/octet-stream", // Content type
                    [data](size_t offset, size_t length, httplib::DataSink& sink)
                    {
                        const auto& d = *data;
                        sink.write((&d + offset), std::min(length, DATA_CHUNK_SIZE));
                        return true; // return 'false' if you want to cancel the process.
                    },
                    [data](bool success) { delete[] data; });

                return true;
            });

    std::cout << "Server started..." << std::endl;
    std::cout << "Enter ..." << std::endl;
    std::cout << "exit - to breaking" << std::endl;
    std::cout << "get - to get file" << std::endl;
    std::cout << " http://127.0.0.1:8080/get?filename=hello" << std::endl;
    std::cout << " http://127.0.0.1:8080/get?filename=hello&size=10" << std::endl;
    std::cout << " http://127.0.0.1:8080/get?filename=hello&beans=10" << std::endl;

    svr.listen("0.0.0.0", std::stoi(argv[1]));
}
