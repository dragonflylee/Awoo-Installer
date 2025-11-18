#include "alipan.hpp"
#include "util/lang.hpp"
#include "util/config.hpp"
#include "ui/MainApplication.hpp"
#include <mbedtls/ecdsa.h>
#include <mbedtls/sha256.h>
#include <mbedtls/base64.h>
#include <regex>

namespace inst::ui {
    extern MainApplication* mainApp;
}

using json = nlohmann::json;

namespace inst::drive {
    const std::string alipan_callback = "https://www.alipan.com/sign/callback";
    const std::string alipan_canary = "X-Canary: client=windows,app=adrive,version=v4.12.0";

    bool alipan::tokenIsValid() {
        // 1. 计算设备ID
        if (this->drive_id.empty()) {
            AccountUid uid;
            uint8_t digest[32];
            if (R_FAILED(accountGetPreselectedUser(&uid))) {
                if (R_FAILED(accountTrySelectUserWithoutInteraction(&uid, false))) {
                    accountGetLastOpenedUser(&uid);
                }
            }
            if (accountUidIsValid(&uid)) {
                sha256CalculateHash(digest, &uid, sizeof(uid));
            } else {
                randomGet(digest, sizeof(digest));
            }
            this->drive_id = hex_encode(digest, sizeof(digest));
        }

        // 2. Open Browser
        WebCommonConfig webCfg;
        WebCommonReply webReply;
        std::string query = HTTP::encode_form({
            {"login_type", "custom"},
            {"response_type", "code"},
            {"redirect_uri", alipan_callback},
            {"client_id", "25dzX3vbYqktVxyX"},
            {"sid", std::to_string(time(nullptr))},
            {"state", "{\"origin\":\"file://\"}"},
        });
        std::string url = "https://auth.alipan.com/v2/oauth/authorize?" + query;

        webPageCreate(&webCfg, url.c_str());
        webConfigSetFooter(&webCfg, false);
        webConfigSetPointer(&webCfg, false);
        webConfigSetPageCache(&webCfg, false);
        webConfigSetJsExtension(&webCfg, true);
        webConfigSetUserAgentAdditionalString(&webCfg, "aDrive/4.12.0");
        webConfigSetCallbackUrl(&webCfg, alipan_callback.c_str());
        if (R_FAILED(webConfigShow(&webCfg, &webReply))) return false;

        size_t lastUrlLen = 0;
        std::string lastUrl(1024, '\0');
        if (R_FAILED(webReplyGetLastUrl(&webReply, lastUrl.data(), lastUrl.size(), &lastUrlLen))) return false;
        lastUrl.resize(lastUrlLen);

        int pos = lastUrl.find("?code=") + 6;
        int end = lastUrl.find_first_of('&', pos);
        if (pos < end) query = lastUrl.substr(pos, end - pos);

        // 3. GetToken
        SetSysDeviceNickName nick;
        setsysGetDeviceNickname(&nick);

        json data = {
            {"code", query},
            {"loginType", "normal"},
            {"deviceId", this->drive_id},
            {"deviceName", nick.nickname},
            {"modelName", "Windows客户端"},
        };

        json resp = this->request("/token/get", data);
        this->access_token = resp.at("access_token");
        this->refresh_token = resp.at("refresh_token");
        this->user_id = resp.at("user_id");
        this->drive_id = resp.at("default_drive_id");

        return true;
    }

    drive::entries alipan::list(const std::string& file_id) {
        json data = {
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
        };
        json j = this->request("/adrive/v3/file/list", data);
        entries list;
        for (auto& item : j.at("items")) {
            if (item.at("type") == "folder") {
                list.push_back({item.at("file_id"), item.at("name"), true});
            } else if (item.contains("download_url")) {
                list.push_back({item.at("download_url"), item.at("name")});
            }
        }
        return list;
    }

    json alipan::request(const std::string& api, const json& data) {
        HTTP s;
        for (;;) {
            std::vector<std::string> headers = {
                "Referer: https://www.alipan.com/",
                "Origin: https://www.alipan.com",
                "Content-Type: application/json",
                alipan_canary,
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
            s.set_headers(headers);
            json j = json::parse(s.post("https://api.alipan.com" + api, data.dump()));

            if (!j.contains("code") || j.at("code").is_null()) return j;

            std::string code = j.at("code");
            if (code == "AccessTokenInvalid") {
                if (this->refresh_token.empty()) throw std::runtime_error(j.at("message"));
                this->refreshToken();
            } else if (code == "DeviceSessionSignatureInvalid") {
                this->createSession();
            } else {
                throw std::runtime_error(j.at("message"));
            }
        }
    }

    bool alipan::refreshToken() {
        HTTP s;
        s.set_headers({"Content-Type: application/json", "x-requested-with: XMLHttpRequest"});
        json data = {{"refresh_token", this->refresh_token}, {"grant_type", "refresh_token"}};
        std::string r = s.post("https://auth.alipan.com/v2/account/token", data.dump());
        json j = json::parse(r);
        if (j.contains("code")) return false;

        this->refresh_token = j.at("refresh_token");
        this->access_token = j.at("access_token");
        return this->createSession();
    }

    bool alipan::getSelfuser() {
        json j = this->request("/v2/user/get", {});
        this->user_id = j.at("user_id");
        this->drive_id = j.at("default_drive_id");
        return this->createSession();
    }

    static inline int mbd_rand(void* rng_state, unsigned char* output, size_t len) {
        randomGet(output, len);
        return 0;
    }

    bool alipan::createSession() {
        std::string msg = "5dde4e1bdf9e4966b387ba58f4b3fdc3:" + this->device_id + ":" + this->user_id + ":0";
        std::string pub_key;

        mbedtls_ecdsa_context ctx_sign;
        mbedtls_ecdsa_init(&ctx_sign);
        // gen secp256k1 keypair
        mbedtls_ecdsa_genkey(&ctx_sign, MBEDTLS_ECP_DP_SECP256K1, mbd_rand, nullptr);
        // mbedtls_ecp_group_load(&ctx_sign.grp, MBEDTLS_ECP_DP_SECP256K1);
        // mbedtls_mpi_read_string(&ctx_sign.d, 16, this->device_id.c_str());
        // mbedtls_ecp_mul(&ctx_sign.grp, &ctx_sign.Q, &ctx_sign.d, &ctx_sign.grp.G, mbd_rand, nullptr);

        // dump public key
        size_t pub_len = 0;
        std::vector<uint8_t> pub(MBEDTLS_ECP_MAX_BYTES, 0);
        mbedtls_ecp_point_write_binary(
            &ctx_sign.grp, &ctx_sign.Q, MBEDTLS_ECP_PF_UNCOMPRESSED, &pub_len, pub.data(), pub.size());
        pub_key = hex_encode(pub.data(), pub_len);

        // sign message
        unsigned char msg_hash[32];
        mbedtls_sha256_ret((uint8_t*)msg.c_str(), msg.size(), msg_hash, 0);

        mbedtls_mpi r, s;
        std::vector<uint8_t> sigdata(MBEDTLS_ECDSA_MAX_LEN, 0);
        mbedtls_mpi_init(&r);
        mbedtls_mpi_init(&s);
        mbedtls_ecdsa_sign(&ctx_sign.grp, &r, &s, &ctx_sign.d, msg_hash, sizeof(msg_hash), mbd_rand, nullptr);

        size_t plen = mbedtls_mpi_size(&r);
        mbedtls_mpi_write_binary(&r, sigdata.data(), plen);
        mbedtls_mpi_write_binary(&s, sigdata.data() + plen, plen);
        sigdata[plen * 2] = 1;
        mbedtls_mpi_free(&r);
        mbedtls_mpi_free(&s);
        mbedtls_ecdsa_free(&ctx_sign);
        this->signature = hex_encode(sigdata.data(), plen * 2 + 1);
        // log_debug("sign ({}), pubkey ({}), msg ({})", this->signature, pub_key, msg);

        SetSysDeviceNickName nick;
        setsysGetDeviceNickname(&nick);

        json data = {
            {"deviceName", nick.nickname},
            {"modelName", "Windows客户端"},
            {"pubKey", pub_key},
        };
        json j = this->request("/users/v1/users/device/create_session", data);

        // store token
        config::gAuthKey = this->access_token;
        config::setConfig();
        return j.at("success").get<bool>();
    }

}  // namespace inst::drive