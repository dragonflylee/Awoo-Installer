#include "util/drive.hpp"
#include "httpdir.hpp"
#include <sstream>
#include <iomanip>

namespace inst::drive {
    drive::ref new_drive(drive_type type) {
        switch (type) {
        case dt_httpdir:
            return std::make_shared<httpdir>();
        default:
            throw std::runtime_error("unsupport drive type");
        }
    }

    std::string hex_encode(const unsigned char* data, size_t len) {
        std::stringstream ss;
        for (size_t i = 0; i < len; i++) ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
        return ss.str();
    }

}  // namespace inst::drive