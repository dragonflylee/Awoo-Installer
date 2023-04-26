#pragma once

#include "util/drive.hpp"
#include "util/curl.hpp"
#include "util/util.hpp"

namespace inst::drive {
    class httpdir : public drive {
    public:
        drive_type getType() { return dt_httpdir; }

        entries list(const std::string& url) {
            std::string response = inst::curl::getRange(url);
            entries urls;
            std::size_t index = 0;
            while (index < response.size()) {
                std::string link;
                auto found = response.find("<a href=\"", index);
                if (found == std::string::npos)
                    break;
                index = found + 9;
                while (index < response.size()) {
                    if (response[index] == '"') {
                        if (link.find("../") == std::string::npos) {
                            if (response[index-1] == '/') {
                                urls.push_back({url + link, inst::util::formatUrlString(link), true});
                            } else {
                                for (auto& ext : knownExts) {
                                    if (link.find(ext) != std::string::npos) {
                                        urls.push_back({ url + link, inst::util::formatUrlString(link)});
                                        break;
                                    }
                                }
                            }
                        }
                        break;
                    }
                    link += response[index++];
                }
            }
            return urls;
        }
    };

}