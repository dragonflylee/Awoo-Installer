#include <fstream>
#include <iomanip>
#include "util/config.hpp"
#include "util/json.hpp"

namespace inst::config {
    std::string sigPatchesUrl;
    std::string lastNetUrl;
    std::vector<std::string> updateInfo;
    int languageSetting;
    bool autoUpdate;
    bool deletePrompt;
    bool gayMode;
    bool ignoreReqVers;
    bool overClock;
    bool usbAck;
    bool validateNCAs;

    void setConfig() {
        nlohmann::json j = {
            {"autoUpdate", autoUpdate},
            {"deletePrompt", deletePrompt},
            {"gayMode", gayMode},
            {"ignoreReqVers", ignoreReqVers},
            {"languageSetting", languageSetting},
            {"overClock", overClock},
            {"sigPatchesUrl", sigPatchesUrl},
            {"usbAck", usbAck},
            {"validateNCAs", validateNCAs},
            {"lastNetUrl", lastNetUrl}
        };
        std::ofstream file(inst::config::configPath);
        file << std::setw(4) << j << std::endl;
    }

    void parseConfig() {
        try {
            std::ifstream file(inst::config::configPath);
            nlohmann::json j;
            file >> j;
            autoUpdate = j["autoUpdate"].get<bool>();
            deletePrompt = j["deletePrompt"].get<bool>();
            gayMode = j["gayMode"].get<bool>();
            ignoreReqVers = j["ignoreReqVers"].get<bool>();
            languageSetting = j["languageSetting"].get<int>();
            overClock = j["overClock"].get<bool>();
            sigPatchesUrl = j["sigPatchesUrl"].get<std::string>();
            usbAck = j["usbAck"].get<bool>();
            validateNCAs = j["validateNCAs"].get<bool>();
            lastNetUrl = j["lastNetUrl"].get<std::string>();
        }
        catch (...) {
            // If loading values from the config fails, we just load the defaults and overwrite the old config
            sigPatchesUrl = "https://sigmapatches.coomer.party/sigpatches.zip";
            languageSetting = 99;
            autoUpdate = true;
            deletePrompt = true;
            gayMode = false;
            ignoreReqVers = true;
            overClock = false;
            usbAck = false;
            validateNCAs = true;
            lastNetUrl = "https://";
            setConfig();
        }
    }
}