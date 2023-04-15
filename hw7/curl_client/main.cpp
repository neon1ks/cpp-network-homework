#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include <cpr/cpr.h>

int main()
{

    const cpr::Response r1 = cpr::Get(cpr::Url{ "http://127.0.0.1:8080" });
    std::cout << "Get " << r1.url << std::endl;
    std::cout << r1.text << std::endl << std::endl;

    const cpr::Response r2 = cpr::Get(cpr::Url{ "http://127.0.0.1:8080/get?filename=hello" });
    std::cout << "Get " << r2.url << std::endl;
    std::cout << r2.text << std::endl << std::endl;
    std::ofstream ofs2("hello-r2", std::ios::binary);
    ofs2 << r2.text;
    ofs2.close();

    const cpr::Response r3 = cpr::Get(cpr::Url{ "http://127.0.0.1:8080/get?filename=hello&size=10" });
    std::cout << "Get " << r3.url << std::endl;
    std::cout << r3.text << std::endl << std::endl;
    std::ofstream ofs3("hello-r3", std::ios::binary);
    ofs3 << r3.text;
    ofs3.close();

    const cpr::Response r4 = cpr::Get(cpr::Url{ "http://127.0.0.1:8080/get?filename=hello&beans=6" });
    std::cout << "Get " << r4.url << std::endl;
    std::cout << r4.text << std::endl << std::endl;
    std::ofstream ofs4("hello-r4", std::ios::binary);
    ofs4 << r4.text;
    ofs4.close();

    const cpr::Response r0 = cpr::Get(cpr::Url{ "http://127.0.0.1:8080/exit" });
    std::cout << "Get " << r0.url << std::endl;
    std::cout << r0.text << std::endl;
}
