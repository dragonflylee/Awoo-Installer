#include "drive.hpp"
#include <locale>
#include <codecvt>
#include <sstream>
#include <iomanip>

namespace inst::drive {

std::string hex_encode(const unsigned char *data, size_t len) {
    std::stringstream ss;
    for (size_t i = 0; i < len; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    return ss.str();
}

}