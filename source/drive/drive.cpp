#include "drive.hpp"
#include "httpindex.hpp"

namespace inst::drive {

static std::map<drive_type, drive::ref> driveMap;

drive::ref new_drive(drive_type type) {
    drive::ref client;
    auto it = driveMap.find(type);
    if (it == driveMap.end()) {
        switch (type) {
        case dt_httpdir:
            client = std::make_shared<httpdir>();
            break;
        default:
            throw std::runtime_error("unsupport drive type");
        }
        driveMap.insert(std::make_pair(type, client));
    } else {
        client = it->second;
    }
    return client;
}

}