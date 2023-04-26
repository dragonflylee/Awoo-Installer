#pragma once

#include <string>
#include <vector>
#include <memory>

namespace inst::drive {
 
    struct drive_entry {
        std::string id;
        std::string name;
        bool folder;
    };

    enum drive_type {
        dt_httpdir,  // Http index
        dt_gdrive,   // Google Drive
    };

    class drive {
    public:
        virtual ~drive() {}
        typedef std::shared_ptr<drive> ref;
        typedef std::vector<drive_entry> entries;

        virtual drive_type getType() = 0;
        virtual entries list(const std::string& file_id) = 0;
        virtual std::string getUrl(const std::string& file_id) { return file_id; }
        virtual std::vector<std::string> headers() { return {}; }
    };

    drive::ref new_drive(drive_type type);

    static const std::vector<std::string> knownExts = {
        ".nsp", ".nsz", ".xci", ".xcz",
    };
}