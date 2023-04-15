#include <iostream>
#include <cstdint>


int main()
{
    uint16_t x = 0x0001;
    std::cout << (*(reinterpret_cast<uint8_t*>(&x)) != 0U ? "little" : "big") << "-endian" << std::endl;
}
