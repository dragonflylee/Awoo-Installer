#include "util/http.hpp"
#include "util/config.hpp"
#include <sstream>

#ifndef CURL_PROGRESSFUNC_CONTINUE
#define CURL_PROGRESSFUNC_CONTINUE 0x10000001
#endif

class curl_error : public std::exception {
public:
    explicit curl_error(CURLcode c) : code(c) {}
    const char* what() const noexcept override { return curl_easy_strerror(this->code); }

private:
    CURLcode code;
};

/// @brief curl context

HTTP::HTTP() : chunk(nullptr) {
    this->easy = curl_easy_init();

    curl_easy_setopt(this->easy, CURLOPT_USERAGENT, "Awoo/" + inst::config::appVersion);
    curl_easy_setopt(this->easy, CURLOPT_FOLLOWLOCATION, 1L);
    // enable all supported built-in compressions
    curl_easy_setopt(this->easy, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(this->easy, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(this->easy, CURLOPT_SSL_VERIFYPEER, 0L);

    curl_easy_setopt(this->easy, CURLOPT_COOKIEJAR, "");
}

HTTP::~HTTP() {
    if (this->chunk != nullptr) curl_slist_free_all(this->chunk);
    if (this->easy != nullptr) curl_easy_cleanup(this->easy);
}

void HTTP::set_headers(const std::vector<std::string>& headers) {
    if (this->chunk != nullptr) {
        curl_slist_free_all(this->chunk);
        this->chunk = nullptr;
    }
    for (auto& h : headers) {
        this->chunk = curl_slist_append(this->chunk, h.c_str());
    }
    curl_easy_setopt(this->easy, CURLOPT_HTTPHEADER, this->chunk);
}

void HTTP::set_progress(curl_xferinfo_callback cb) { curl_easy_setopt(this->easy, CURLOPT_XFERINFOFUNCTION, cb); }

void HTTP::set_timeout(long timeout) {
    curl_easy_setopt(this->easy, CURLOPT_TIMEOUT_MS, timeout);
    curl_easy_setopt(this->easy, CURLOPT_CONNECTTIMEOUT_MS, timeout);
}

void HTTP::set_range(long first, long second) {
    std::string ourRange = std::to_string(first) + "-" + std::to_string(second);
    curl_easy_setopt(this->easy, CURLOPT_RANGE, ourRange.c_str());
}

size_t HTTP::easy_write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    std::ostream* ctx = reinterpret_cast<std::ostream*>(userdata);
    size_t count = size * nmemb;
    ctx->write(ptr, static_cast<std::streamsize>(count));
    return count;
}

size_t HTTP::easy_read_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    std::istream* ctx = reinterpret_cast<std::istream*>(userdata);
    size_t count = size * nmemb;
    ctx->read(ptr, static_cast<std::streamsize>(count));
    return ctx->gcount();
}

int HTTP::perform(std::ostream* body) {
    curl_easy_setopt(this->easy, CURLOPT_WRITEFUNCTION, easy_write_cb);
    curl_easy_setopt(this->easy, CURLOPT_WRITEDATA, body);

    CURLcode res = curl_easy_perform(this->easy);
    if (res != CURLE_OK) {
        body->clear();
        throw curl_error(res);
    }
    int status_code = 0;
    curl_easy_getinfo(this->easy, CURLINFO_RESPONSE_CODE, &status_code);
    return status_code;
}

std::string HTTP::encode_form(const Form& form) {
    std::ostringstream ss;
    char* escaped;
    for (auto it = form.begin(); it != form.end(); ++it) {
        if (it != form.begin()) ss << '&';
        escaped = curl_escape(it->second.c_str(), it->second.size());
        ss << it->first << '=' << escaped;
        curl_free(escaped);
    }
    return ss.str();
}

int HTTP::get(const std::string& url, std::ostream* out) {
    out->clear();
    curl_easy_setopt(this->easy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(this->easy, CURLOPT_HTTPGET, 1L);
    return this->perform(out);
}

std::string HTTP::put(const std::string& url, std::istream* data) {
    std::ostringstream body;
    curl_easy_setopt(this->easy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(this->easy, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(this->easy, CURLOPT_READFUNCTION, easy_read_cb);
    curl_easy_setopt(this->easy, CURLOPT_READDATA, data);
    this->perform(&body);
    return body.str();
}

std::string HTTP::post(const std::string& url, const std::string& data) {
    std::ostringstream body;
    curl_easy_setopt(this->easy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(this->easy, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(this->easy, CURLOPT_POSTFIELDSIZE, data.size());
    this->perform(&body);
    return body.str();
}