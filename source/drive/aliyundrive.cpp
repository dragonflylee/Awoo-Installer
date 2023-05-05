#include "aliyundrive.hpp"
#include "util/lang.hpp"
#include "util/config.hpp"
#include "ui/MainApplication.hpp"
#include <mbedtls/ecdsa.h>
#include <mbedtls/sha256.h>
#include <mbedtls/base64.h>
#include <regex>

namespace inst::ui {
    extern MainApplication *mainApp;
}

using json = nlohmann::json;

namespace inst::drive {

const std::string aliyundrive_get_qrcode = "https://passport.aliyundrive.com/newlogin/qrcode/generate.do?appName=aliyun_drive&fromSite=52";
const std::string aliyundrive_check_qrcode = "https://passport.aliyundrive.com/newlogin/qrcode/query.do?appName=aliyun_drive&fromSite=52";
const std::string aliyundrive_pre_auth =
    "https://auth.aliyundrive.com/v2/oauth/authorize?client_id=25dzX3vbYqktVxyX"
    "&redirect_uri=https%3A%2F%2Fwww.aliyundrive.com%2Fsign%2Fcallback&response_type=code&login_type=custom"
    "&state=%7B%22origin%22%3A%22https%3A%2F%2Fwww.aliyundrive.com%22%7D";

drive::entries aliyundrive::list(const std::string& file_id) {
    json j = this->request("/adrive/v3/file/list", {
        {"drive_id", this->drive_id},
        {"parent_file_id", file_id},
        {"all", true},
        {"url_expire_sec", 14400},
        {"image_thumbnail_process", "image/resize,w_256/format,jpeg"},
        {"image_url_process", "image/resize,w_1920/format,jpeg/interlace,1"},
        {"video_thumbnail_process", "video/snapshot,t_1000,f_jpg,ar_auto,w_256"},
        {"fields", "*"},
        {"order_by", "updated_at"},
        {"order_direction", "DESC"},
    });
    entries list;
    for (auto& item : j.at("items")) {
        if (item.at("type") == "folder") {
            list.push_back({ item.at("file_id"), item.at("name"), true });
        } else if (item.contains("download_url")) {
            list.push_back({ item.at("download_url"), item.at("name") });
        }
    }
    return list;
}

drive_status aliyundrive::qrLogin(onQrcode printQr) {
    if (config::aliDriveToken.size() > 0) {
        this->access_token = config::aliDriveToken;
        if (this->getSelfuser() == ds_ok) {
            return ds_ok;
        }
    }

    std::vector<std::string> headers = {
        "Referer: https://www.aliyundrive.com/",
        "X-Canary: client=windows,app=adrive,version=v4.3.0",
    };
    this->get(aliyundrive_pre_auth, headers);

    // generate qrcode
    json j = json::parse(this->get(aliyundrive_get_qrcode, headers));
    json qr = j.at("content").at("data");

    std::string codeContent = qr.at("codeContent");
    printQr(codeContent, false);

    std::string query = this->formEncode({
        { "t", std::to_string(qr.at("t").get<long>()) },
        { "ck", qr.at("ck") },
        { "appName", "aliyun_drive" },
        { "appEntrance", "web" },
        { "isMobile", "false" },
        { "lang", "zh_CN" },
        { "returnUrl", "" },
        { "fromSite", "52" },
        { "bizParams", "" }
    });

    u64 freq = armGetSystemTickFreq();
    u64 renderTime = armGetSystemTick();
    u64 qrTime = armGetSystemTick();
    bool scaned = false;
    while (true) {
        // If we don't update the UI occasionally the Switch basically crashes on this screen if you press the home button
        u64 newTime = armGetSystemTick();
        if (newTime - renderTime >= freq * 0.25) {
            renderTime = newTime;
            inst::ui::mainApp->CallForRender();
        }

        // Break on input pressed
        inst::ui::mainApp->UpdateButtons();
        u64 kDown = inst::ui::mainApp->GetButtonsDown();

        if (kDown & HidNpadButton_B) {
            return ds_canceled;
        }

        if (newTime - qrTime >= freq * 2) {
            qrTime = newTime;
            std::string qrCodeStatus = this->checkQrcode(query);
            if (qrCodeStatus == "NEW") {
            } else if (qrCodeStatus == "SCANED") {
                if (!scaned) {
                    scaned = true;
                    printQr(codeContent, true);
                }
            } else if (qrCodeStatus == "EXPIRED") {
                return ds_expired;
            } else if (qrCodeStatus == "CANCELED") {
                return ds_canceled;
            } else if (qrCodeStatus == "CONFIRMED") {
                return this->confirmLogin();
            }
        }
    }
}

std::string aliyundrive::checkQrcode(const std::string& query) {
    json j = json::parse(this->post(aliyundrive_check_qrcode, query, {
        "Content-Type: application/x-www-form-urlencoded",
        "X-Canary: client=windows,app=adrive,version=v4.3.0",
        "Origin: https://passport.aliyundrive.com",
    }));
    json data = j["content"]["data"];
    std::string qrCodeStatus = data.at("qrCodeStatus");
    if (qrCodeStatus == "CONFIRMED") {
        // decode token
        size_t out_len = 0;
        std::string bizExt = data.at("bizExt");
        std::vector<char> gbkBiz(bizExt.size());
        mbedtls_base64_decode((uint8_t*)gbkBiz.data(), gbkBiz.size(), &out_len, (const uint8_t*)bizExt.c_str(), bizExt.size());

        static std::regex remove_re("[^\\x20-\\x7f]");
        bizExt = std::regex_replace(std::string(gbkBiz.data(), out_len), remove_re, "");        
        json login = json::parse(bizExt).at("pds_login_result");
        this->user_id = login.at("userId");
        // calculate device id
        unsigned char digest[32];
        mbedtls_sha256_ret((uint8_t*)this->user_id.c_str(), this->user_id.size(), digest, 0);
        this->device_id = hex_encode(digest, sizeof(digest));
        // get token
        this->drive_id = login.at("defaultDriveId");
        this->access_token = login.at("accessToken");
        this->refresh_token = login.at("refreshToken");
    }
    return qrCodeStatus;
}

json aliyundrive::request(const std::string& api, const json& data) {
    std::vector<std::string> headers = {
        "Referer: https://www.aliyundrive.com/",
        "Origin: https://www.aliyundrive.com",
        "Content-Type: application/json",
        "X-Canary: client=windows,app=adrive,version=v4.3.0",
    };
    if (this->access_token.size() > 0) {
        headers.push_back("Authorization: Bearer " + this->access_token);
    }
    if (this->device_id.size() > 0) {
        headers.push_back("X-Device-Id: " + this->device_id);
    }
    if (this->signature.size() > 0) {
        headers.push_back("X-Signature: " + this->signature);
    }
    return json::parse(this->post("https://api.aliyundrive.com" + api, data.dump(), headers));
}

drive_status aliyundrive::getSelfuser() {
    json j = this->request("/v2/user/get", {});
    if (j.contains("code")) {
        return ds_error;
    }

    this->user_id = j.at("user_id");
    this->drive_id = j.at("default_drive_id");
    if (this->device_id.empty()) {
        unsigned char digest[32];
        mbedtls_sha256_ret((uint8_t*)this->user_id.c_str(), this->user_id.size(), digest, 0);
        this->device_id = hex_encode(digest, sizeof(digest));
    }
    return ds_ok;
}

drive_status aliyundrive::confirmLogin() {
    json data = {{"token", this->access_token}};
    std::string r = this->post("https://auth.aliyundrive.com/v2/oauth/token_login", data.dump(),
    {
        "Referer: https://www.aliyundrive.com/",
        "Origin: https://www.aliyundrive.com",
        "Content-Type: application/json",
        "X-Canary: client=Android,app=adrive,version=v4.3.0",
        "X-Device-Id: " + this->device_id,
    });
    json j = json::parse(r);
    if (!j.contains("goto")) {
        return ds_error;
    }
    return this->createSession();
}

static inline int mbd_rand(void *rng_state, unsigned char *output, size_t len) {
    randomGet(output, len);
    return 0;
}

drive_status aliyundrive::createSession() {
    std::string msg = "5dde4e1bdf9e4966b387ba58f4b3fdc3:" + this->device_id + ":" + this->user_id + ":0";

    mbedtls_ecdsa_context ctx_sign;
    mbedtls_ecdsa_init(&ctx_sign);
    mbedtls_ecp_group_load(&ctx_sign.grp, MBEDTLS_ECP_DP_SECP256K1);
    mbedtls_mpi_read_string(&ctx_sign.d, 16, this->device_id.c_str());
    mbedtls_ecp_mul(&ctx_sign.grp, &ctx_sign.Q, &ctx_sign.d, &ctx_sign.grp.G, nullptr, nullptr);
    // dump public key
    size_t pub_len = 0;
    std::vector<uint8_t> pub(MBEDTLS_ECP_MAX_BYTES, 0);
    mbedtls_ecp_point_write_binary(&ctx_sign.grp, &ctx_sign.Q, MBEDTLS_ECP_PF_UNCOMPRESSED, &pub_len, pub.data(), pub.size());
    // sign message
    unsigned char msg_hash[32];
    std::vector<uint8_t> sigdata(MBEDTLS_ECDSA_MAX_LEN, 0);
    mbedtls_mpi r, s;
    mbedtls_sha256_ret((uint8_t*)msg.c_str(), msg.size(), msg_hash, 0);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);
    mbedtls_ecdsa_sign(&ctx_sign.grp, &r, &s, &ctx_sign.d, msg_hash, sizeof(msg_hash), mbd_rand, nullptr);
    size_t plen = mbedtls_mpi_size(&r);
    mbedtls_mpi_write_binary(&r, sigdata.data(), plen);
    mbedtls_mpi_write_binary(&s, sigdata.data() + plen, plen);
    sigdata[plen * 2] = 1;
    this->signature = hex_encode(sigdata.data(), plen * 2 + 1);
    mbedtls_ecdsa_free(&ctx_sign);

    SetSysDeviceNickName nick;
    std::string deviceName = "Switch";
    if (R_SUCCEEDED(setsysGetDeviceNickname(&nick))) {
        deviceName = nick.nickname;
    }
    json j = this->request("/users/v1/users/device/create_session", {
        {"deviceName", deviceName},
        {"modelName", "Windows客户端"},
        {"pubKey", hex_encode(pub.data(), pub_len)},
    });

    // store token
    config::aliDriveToken = this->access_token;
    config::setConfig();
    return j.at("success").get<bool>() ? ds_ok : ds_error;
}

}