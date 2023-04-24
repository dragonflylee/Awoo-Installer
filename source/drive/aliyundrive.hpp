#pragma once

#include "util/drive.hpp"
#include "util/curl.hpp"
#include "util/json.hpp"

namespace inst::drive {
    class aliyundrive : public drive, public curl::client {
    public:
        const char* userAgent() { return "aDrive/4.3.0"; }
        drive_type getType() { return dt_alidrive; }

        entries list(const std::string& file_id);
        drive_status qrLogin(onQrcode printQr);

    private:
        std::string checkQrcode(const std::string& query);
        drive_status getSelfuser();
        drive_status confirmLogin();

        nlohmann::json request(const std::string& api, const nlohmann::json& j);

        std::string user_id;
        std::string device_id;
        std::string signature;
        std::string drive_id;
        std::string access_token;
        std::string refresh_token;
    };
}