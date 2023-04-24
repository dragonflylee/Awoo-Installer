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
        dt_alidrive, // AliyunDrive
    };

    class drive {
    public:
        virtual ~drive() {}
        typedef std::shared_ptr<drive> ref;
        typedef std::vector<drive_entry> entries;

        using onQrcode = std::function<void(const std::string&, bool)>;
        virtual drive_type getType() = 0;
        virtual drive_status qrLogin(onQrcode printQr) { return ds_ok; }
        virtual entries list(const std::string& file_id) = 0;
    };

    drive::ref new_drive(drive_type type);

    void renderQr(const std::string& text, pu::ui::elm::Image::Ref& image);
    std::string hex_encode(const unsigned char* data, size_t len);

    static const std::vector<std::string> knownExts = {
        ".nsp", ".nsz", ".xci", ".xcz",
    };
}