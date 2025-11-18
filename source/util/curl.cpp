#include <curl/curl.h>
#include <string>
#include <sstream>
#include <iostream>
#include "util/curl.hpp"
#include "util/http.hpp"
#include "util/config.hpp"
#include "util/error.hpp"
#include "ui/instPage.hpp"

int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    if (ultotal) {
        int uploadProgress = (int)(((double)ulnow / (double)ultotal) * 100.0);
        inst::ui::instPage::setInstBarPerc(uploadProgress);
    } else if (dltotal) {
        int downloadProgress = (int)(((double)dlnow / (double)dltotal) * 100.0);
        inst::ui::instPage::setInstBarPerc(downloadProgress);
    }
    return 0;
}

namespace inst::curl {
    bool downloadFile(const std::string ourUrl, const char* pagefilename, long timeout, bool writeProgress) {
        HTTP client;
        std::ofstream ss(pagefilename);
        if (!ss.is_open()) return false;
        client.set_timeout(timeout);
        if (writeProgress) client.set_progress(progress_callback);
        if (client.get(ourUrl, &ss) != 200) return false;
        return true;
    }

    std::string downloadToBuffer(const std::string ourUrl, int firstRange, int secondRange, long timeout) {
        HTTP client;
        std::ostringstream ss;
        client.set_range(firstRange, secondRange);
        client.set_timeout(timeout);
        client.get(ourUrl, &ss);
        return ss.str();
    }
}  // namespace inst::curl