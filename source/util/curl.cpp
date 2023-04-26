#include <curl/curl.h>
#include <string>
#include <sstream>
#include <iostream>
#include "util/curl.hpp"
#include "util/config.hpp"
#include "util/error.hpp"
#include "ui/instPage.hpp"

namespace inst::curl {

    const std::string default_user_agent = "Awoo-Installer/" + inst::config::appVersion;

    struct curl_global {
        curl_global() {
            ASSERT_OK(curl_global_init(CURL_GLOBAL_ALL), "Curl failed to initialized");
        }

        ~curl_global() {
            curl_global_cleanup();
        }
    };

    struct curl_ctx {
        CURL* easy;
        struct curl_slist* headers;
        long timeout = -1;
        int status_code = 0;

        curl_ctx(): headers(nullptr) {
            static curl_global global;
            easy = curl_easy_init();
        }

        ~curl_ctx() {
            if (headers != nullptr)
                curl_slist_free_all(headers);
            if (easy != nullptr)
                curl_easy_cleanup(easy);
        }

        operator CURL*() const {
            return this->easy;
        }

        void set_headers(const std::vector<std::string>& hs) {
            if (this->headers != nullptr) {
                curl_slist_free_all(this->headers);
                this->headers = nullptr;
            }
            for (auto& h : hs) {
                this->headers = curl_slist_append(this->headers, h.c_str());
            }
        }

        static size_t easy_write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
            std::ostream* ctx = reinterpret_cast<std::ostream*>(userdata);
            size_t count = size * nmemb;
            ctx->write(ptr, count);
            return count;
        }

        CURLcode perform(std::ostream* body) {
            curl_easy_setopt(this->easy, CURLOPT_FOLLOWLOCATION, 1L);
            // enable all supported built-in compressions
            curl_easy_setopt(this->easy, CURLOPT_ACCEPT_ENCODING, "");
            curl_easy_setopt(this->easy, CURLOPT_SSL_VERIFYHOST, 0L);
            curl_easy_setopt(this->easy, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(this->easy, CURLOPT_NOPROGRESS, 1L);
            curl_easy_setopt(this->easy, CURLOPT_TIMEOUT_MS, timeout);
            curl_easy_setopt(this->easy, CURLOPT_CONNECTTIMEOUT_MS, timeout);
            curl_easy_setopt(this->easy, CURLOPT_WRITEFUNCTION, easy_write_cb);
            curl_easy_setopt(this->easy, CURLOPT_WRITEDATA, body);
            curl_easy_setopt(this->easy, CURLOPT_COOKIEJAR, "");

            if (this->headers != nullptr) {
                curl_easy_setopt(this->easy, CURLOPT_HTTPHEADER, this->headers);
            }

            CURLcode result = curl_easy_perform(easy);
            if (result != CURLE_OK) {
                body->clear();
                LOG_DEBUG(curl_easy_strerror(result));
            }
            curl_easy_getinfo(this->easy, CURLINFO_RESPONSE_CODE, &this->status_code);
            return result;
        }
    };

    client::client() : ctx(new curl_ctx) {}

    client::~client() { delete this->ctx; }

    std::string client::get(const std::string& url, const std::vector<std::string>& headers) {
        std::ostringstream body;
        ctx->set_headers(headers);
        curl_easy_setopt(ctx->easy, CURLOPT_URL, url.c_str());
        curl_easy_setopt(ctx->easy, CURLOPT_USERAGENT, this->userAgent());
        curl_easy_setopt(ctx->easy, CURLOPT_HTTPGET, 1L);
        ctx->perform(&body);
        return body.str();
    }

    std::string client::post(const std::string& url, const std::string& data, const std::vector<std::string>& headers) {
        std::ostringstream body;
        ctx->set_headers(headers);
        curl_easy_setopt(ctx->easy, CURLOPT_URL, url.c_str());
        curl_easy_setopt(ctx->easy, CURLOPT_USERAGENT, this->userAgent());
        curl_easy_setopt(ctx->easy, CURLOPT_POST, 1L);
        curl_easy_setopt(ctx->easy, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(ctx->easy, CURLOPT_POSTFIELDSIZE, data.size());
        ctx->perform(&body);
        return body.str();
    }

    std::string client::formEncode(const std::map<std::string, std::string>& form) {
        std::ostringstream ss;
        char* escaped;
        for (auto it : form) {
            escaped = curl_easy_escape(ctx->easy, it.second.c_str(), it.second.size());
            ss << it.first << '=' << escaped << '&';
            curl_free(escaped);
        }
        return ss.str();
    }

    int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
        if (ultotal) {
            int uploadProgress = (int)(((double)ulnow / (double)ultotal) * 100.0);
            inst::ui::instPage::setInstBarPerc(uploadProgress);
        } else if (dltotal) {
            int downloadProgress = (int)(((double)dlnow / (double)dltotal) * 100.0);
            inst::ui::instPage::setInstBarPerc(downloadProgress);
        }
        return 0;
    }

    bool downloadFile(const std::string ourUrl, const char *pagefilename, long timeout, bool writeProgress) {
        curl_ctx curl_handle;
        std::ofstream pagefile(pagefilename);

        curl_easy_setopt(curl_handle, CURLOPT_URL, ourUrl.c_str());
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, default_user_agent.c_str());
        if (writeProgress)
            curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, progress_callback);
    
        if (curl_handle.perform(&pagefile) == CURLE_OK) 
            return true;
        
        LOG_DEBUG(curl_easy_strerror(result));
        return false;
    }

    std::string getRange(const std::string ourUrl, int firstRange, int secondRange, long timeout) {
        curl_ctx curl_handle;
        std::ostringstream stream;
        curl_easy_setopt(curl_handle, CURLOPT_URL, ourUrl.c_str());
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, default_user_agent.c_str());

        if (firstRange && secondRange) {
            std::string ourRange = std::to_string(firstRange) + "-" + std::to_string(secondRange);
            curl_easy_setopt(curl_handle, CURLOPT_RANGE, ourRange.c_str());
        }

        curl_handle.perform(&stream);
        return stream.str();
    }
}