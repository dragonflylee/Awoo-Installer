#include "drive.hpp"
#include "httpindex.hpp"

namespace inst::drive {

drive::ref new_drive(drive_type type) {
    switch (type) {
    case dt_httpdir:
        return std::make_unique<httpdir>();
    default:
        return nullptr;
    }
}

}