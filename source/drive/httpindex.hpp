#pragma once

#include "util/drive.hpp"

namespace inst::drive {
    class httpdir : public drive {
    public:
        drive_type getType() { return dt_httpdir; }

        entries list(const std::string& url);
    };

}