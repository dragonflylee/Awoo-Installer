#include "httpindex.hpp"
#include "util/curl.hpp"
#include "util/util.hpp"
#include "util/config.hpp"

namespace inst::drive
{
    static const std::vector<std::string> knownExts = {
        ".nsp",
        ".nsz",
        ".xci",
        ".xcz",
    };

    drive::entries httpdir::list(const std::string &url)
    {
        // if url is single file, start install
        for (auto& ext : knownExts) {
            if (url.find(ext) != std::string::npos) {
                return { { .id = url } };
            }
        }

        config::lastNetUrl = url.back() != '/' ? url + '/' : url;
        config::setConfig();

        std::string response = inst::curl::getRange(config::lastNetUrl);
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
                        if (response[index - 1] == '/') {
                            urls.push_back({url + link, inst::util::formatUrlString(link), true});
                        } else {
                            for (auto &ext : knownExts)  {
                                if (link.find(ext) != std::string::npos) {
                                    urls.push_back({url + link, inst::util::formatUrlString(link)});
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

}