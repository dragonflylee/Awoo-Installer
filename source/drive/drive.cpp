#include "drive.hpp"
#include "httpindex.hpp"
#include "aliyundrive.hpp"

namespace inst::drive {

drive::ref new_drive(drive_type type) {
    switch (type) {
    case dt_httpdir:
        return std::make_shared<httpdir>();
    case dt_alidrive:
        return std::make_shared<aliyundrive>();
    default:
        throw std::runtime_error("unsupport drive type");
    }
}

}