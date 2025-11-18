/*
    Copyright 2023 dragonflylee
*/

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <curl/curl.h>

class HTTP {
public:
    using Form = std::unordered_map<std::string, std::string>;

    HTTP();
    HTTP(const HTTP& other) = delete;
    ~HTTP();

    static std::string encode_form(const Form& form);
    void set_headers(const std::vector<std::string>& headers);
    void set_progress(curl_xferinfo_callback cb);
    void set_timeout(long timeout);
    void set_range(long first, long second);
    int get(const std::string& url, std::ostream* out);
    std::string put(const std::string& url, std::istream* data);
    std::string post(const std::string& url, const std::string& data);

private:
    static size_t easy_write_cb(char* ptr, size_t size, size_t nmemb, void* userdata);
    static size_t easy_read_cb(char* ptr, size_t size, size_t nmemb, void* userdata);
    int perform(std::ostream* body);

    CURL* easy;
    struct curl_slist* chunk;
};
