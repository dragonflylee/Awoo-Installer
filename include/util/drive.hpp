#pragma once

#include <pu/ui/elm/elm_Image.hpp>
#include <functional>

namespace inst::drive {

    enum drive_status {
        ds_ok,
        ds_expired,
        ds_canceled,
        ds_error,
    };

    struct drive_entry {
        std::string id;
        std::string name;
        bool folder;
    };

    enum drive_type {
        dt_httpdir,  // Http index
        dt_gdrive,   // Google Drive
        dt_alipan,   // Aliyun Drive
    };

    class drive {
    public:
        virtual ~drive() {}
        typedef std::shared_ptr<drive> ref;
        typedef std::vector<drive_entry> entries;

        virtual drive_type getType() = 0;
        virtual entries list(const std::string& file_id) = 0;
    };

    drive::ref new_drive(drive_type type);
    std::string hex_encode(const unsigned char* data, size_t len);

}  // namespace inst::drive