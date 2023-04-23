#pragma once

#include <string>
#include <vector>
#include <memory>

namespace inst::drive {
 
    struct entry {
        std::string id;
        std::string name;
        bool folder;
    };

    typedef std::vector<entry> entries;

    class drive {
    public:
        virtual ~drive() {}
        typedef std::unique_ptr<drive> ref;

        virtual entries list(const std::string& file_id) = 0;
        virtual std::string getUrl(const std::string& file_id) { return file_id; }
        virtual std::vector<std::string> headers() { return {}; }
    };

    enum drive_type {
        dt_httpdir,  // Http index
        dt_gdrive,   // Google Drive
    };

    drive::ref new_drive(drive_type type);

    static const std::vector<std::string> knownExts = {
        ".nsp", ".nsz", ".xci", ".xcz",
    };
}