#pragma once

#include "util/drive.hpp"
#include "util/http.hpp"
#include "util/json.hpp"

namespace inst::drive {
    class alipan : public drive {
    public:
        drive_type getType() override { return dt_alipan; }

        entries list(const std::string& file_id) override;

    private:
        bool getSelfuser();
        bool createSession();
        bool refreshToken();
        bool tokenIsValid();

        nlohmann::json request(const std::string& api, const nlohmann::json& j);

        std::string user_id;
        std::string device_id;
        std::string signature;
        std::string drive_id;
        std::string access_token;
        std::string refresh_token;
    };
}