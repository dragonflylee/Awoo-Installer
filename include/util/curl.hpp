#pragma once

#include <string>
#include <vector>
#include <map>

namespace inst::curl {
    extern const std::string default_user_agent;

    bool downloadFile(const std::string ourUrl, const char *pagefilename, long timeout = 5000, bool writeProgress = false);
    std::string getRange(const std::string ourUrl, int firstRange = -1, int secondRange = -1, long timeout = 5000);

    struct curl_ctx;

    class client {
    public:
        client();
        virtual ~client();
        virtual const char* userAgent() { return default_user_agent.c_str(); }

        std::string get(const std::string& url, const std::vector<std::string>& headers);
        std::string post(const std::string& url, const std::string& data, const std::vector<std::string>& headers);
        std::string formEncode(const std::map<std::string, std::string>& form);

    private:
        curl_ctx* ctx;
    };

}