#include <filesystem>
#include <switch.h>
#include "ui/MainApplication.hpp"
#include "ui/mainPage.hpp"
#include "ui/netInstPage.hpp"
#include "util/util.hpp"
#include "util/config.hpp"
#include "util/curl.hpp"
#include "util/lang.hpp"
#include "netInstall.hpp"

#define COLOR(hex) pu::ui::Color::FromHex(hex)

namespace inst::ui {
    extern MainApplication *mainApp;

    drive::drive::ref client;
    std::string sourceString = "";

    netInstPage::netInstPage() : Layout::Layout() {
        this->SetBackgroundColor(COLOR("#670000FF"));
        if (std::filesystem::exists(inst::config::appDir + "/background.png")) this->SetBackgroundImage(inst::config::appDir + "/background.png");
        else this->SetBackgroundImage("romfs:/images/background.jpg");
        this->topRect = Rectangle::New(0, 0, 1280, 94, COLOR("#170909FF"));
        this->infoRect = Rectangle::New(0, 95, 1280, 60, COLOR("#17090980"));
        this->botRect = Rectangle::New(0, 660, 1280, 60, COLOR("#17090980"));
        if (inst::config::gayMode) {
            this->titleImage = Image::New(-113, 0, "romfs:/images/logo.png");
            this->appVersionText = TextBlock::New(367, 49, "v" + inst::config::appVersion, 22);
        }
        else {
            this->titleImage = Image::New(0, 0, "romfs:/images/logo.png");
            this->appVersionText = TextBlock::New(480, 49, "v" + inst::config::appVersion, 22);
        }
        this->appVersionText->SetColor(COLOR("#FFFFFFFF"));
        this->pageInfoText = TextBlock::New(10, 109, "", 30);
        this->pageInfoText->SetColor(COLOR("#FFFFFFFF"));
        this->butText = TextBlock::New(10, 678, "", 24);
        this->butText->SetColor(COLOR("#FFFFFFFF"));
        this->menu = pu::ui::elm::Menu::New(0, 156, 1280, COLOR("#FFFFFF00"), 84, (506 / 84));
        this->menu->SetOnFocusColor(COLOR("#00000033"));
        this->menu->SetScrollbarColor(COLOR("#17090980"));
        this->infoImage = Image::New(453, 292, "romfs:/images/icons/lan-connection-waiting.png");
        this->Add(this->topRect);
        this->Add(this->infoRect);
        this->Add(this->botRect);
        this->Add(this->titleImage);
        this->Add(this->appVersionText);
        this->Add(this->butText);
        this->Add(this->pageInfoText);
        this->Add(this->menu);
        this->Add(this->infoImage);
    }

    void netInstPage::drawMenuItems(bool clearItems) {
        if (clearItems) {
            this->selectedUrls.clear();
            this->alternativeNames.clear();
        }
        this->menu->ClearItems();
        if (this->lastFileId.size() > 1) {
            std::string itm = "..";
            auto ourEntry = pu::ui::elm::MenuItem::New(itm);
            ourEntry->SetColor(COLOR("#FFFFFFFF"));
            ourEntry->SetIcon("romfs:/images/icons/folder-upload.png");
            this->menu->AddItem(ourEntry);
        }
        for (auto& url: this->ourUrls) {
            std::string itm = inst::util::shortenString(url.name, 56, true);
            auto ourEntry = pu::ui::elm::MenuItem::New(itm);
            ourEntry->SetColor(COLOR("#FFFFFFFF"));
            if (url.folder) {
                ourEntry->SetIcon("romfs:/images/icons/folder.png");
            } else {
                ourEntry->SetIcon("romfs:/images/icons/checkbox-blank-outline.png");
                for (long unsigned int i = 0; i < this->selectedUrls.size(); i++) {
                    if (this->selectedUrls[i] == url.id) {
                        ourEntry->SetIcon("romfs:/images/icons/check-box-outline.png");
                    }
                }
            }
            this->menu->AddItem(ourEntry);
        }
    }

    void netInstPage::selectTitle(int selectedIndex) {
        std::string fileId;
        int dirListSize = this->lastFileId.size() > 1 ? 1 : 0;
        if (selectedIndex < dirListSize) {
            this->lastFileId.pop_back();
            fileId = this->lastFileId.back();
        } else {
            fileId = this->ourUrls[selectedIndex - dirListSize].id;
        }

        if (this->menu->GetItems()[selectedIndex]->GetIcon() == "romfs:/images/icons/check-box-outline.png") {
            for (long unsigned int i = 0; i < this->selectedUrls.size(); i++) {
                if (this->selectedUrls[i] == fileId)
                    this->selectedUrls.erase(this->selectedUrls.begin() + i);
            }
            this->drawMenuItems(false);
        } else if (this->menu->GetItems()[selectedIndex]->GetIcon() == "romfs:/images/icons/checkbox-blank-outline.png") {
            this->selectedUrls.push_back(fileId);
            this->drawMenuItems(false);
        } else {
            this->ourUrls = client->list(fileId);
            if (selectedIndex >= dirListSize) {
                this->lastFileId.push_back(fileId);
            }
            this->drawMenuItems(true);
        }
    }

    void netInstPage::startNetwork() {
        std::vector<std::string> options = {"inst.net.src.opt0"_lang, "inst.net.src.opt1"_lang};
        std::string url;
    
        while (true) {
            this->butText->SetText("inst.net.buttons"_lang);
            this->menu->SetVisible(false);
            this->menu->ClearItems();
            this->infoImage->SetVisible(true);
            mainApp->LoadLayout(mainApp->netinstPage);
            this->ourUrls.clear();
            this->selectedUrls.clear();
            
            auto urls = netInstStuff::OnSelected();
            if (urls.empty()) {
                mainApp->LoadLayout(mainApp->mainPage);
                return;
            }
            if (urls[0] == "supplyUrl") {
                switch (mainApp->CreateShowDialog("inst.net.src.title"_lang, "common.cancel_desc"_lang, options, false)) {
                case 0:
                    url = inst::util::softwareKeyboard("inst.net.url.hint"_lang, inst::config::lastNetUrl, 500);
                    if (url.empty()) continue;

                    if (inst::util::formatUrlString(url) == "" || url == "https://" || url == "http://") {
                        mainApp->CreateShowDialog("inst.net.url.invalid"_lang, "", {"common.ok"_lang}, false);
                        continue;
                    }
                    // if url is single file, start install
                    for (auto& ext : drive::knownExts) {
                        if (url.find(ext) != std::string::npos) {
                            this->selectedUrls = { url };
                            this->startInstall(true);
                            return;
                        }
                    }
                    if (url.back() != '/') {
                        url += '/';
                    }
                    inst::config::lastNetUrl = url;
                    inst::config::setConfig();

                    client = drive::new_drive(drive::dt_httpdir);
                    this->ourUrls = client->list(url);

                    if (this->ourUrls.empty()) {
                        inst::ui::mainApp->CreateShowDialog("inst.net.http.invalid"_lang, "", {"common.ok"_lang}, false);
                        continue;
                    }
                    this->lastFileId.push_back(url);
                    break;

                default:
                    continue;
                }
            } else {
                for (std::string url : urls) {
                    this->ourUrls.push_back({ id: url, name: inst::util::formatUrlString(url) });
                }
            }

            mainApp->CallForRender(); // If we re-render a few times during this process the main screen won't flicker
            sourceString = "inst.net.source_string"_lang;
            this->pageInfoText->SetText("inst.net.top_info"_lang);
            this->butText->SetText("inst.net.buttons1"_lang);
            this->drawMenuItems(true);
            this->menu->SetSelectedIndex(0);
            mainApp->CallForRender();
            this->infoImage->SetVisible(false);
            this->menu->SetVisible(true);
            return;
        }
    }

    void netInstPage::startInstall(bool urlMode) {
        int dialogResult = -1;
        if (this->selectedUrls.size() == 1) {
            std::string ourUrlString;
            if (this->alternativeNames.size() > 0) ourUrlString = inst::util::shortenString(this->alternativeNames[0], 32, true);
            else ourUrlString = inst::util::shortenString(inst::util::formatUrlString(this->selectedUrls[0]), 32, true);
            dialogResult = mainApp->CreateShowDialog("inst.target.desc0"_lang + ourUrlString + "inst.target.desc1"_lang, "common.cancel_desc"_lang, {"inst.target.opt0"_lang, "inst.target.opt1"_lang}, false);
        } else dialogResult = mainApp->CreateShowDialog("inst.target.desc00"_lang + std::to_string(this->selectedUrls.size()) + "inst.target.desc01"_lang, "common.cancel_desc"_lang, {"inst.target.opt0"_lang, "inst.target.opt1"_lang}, false);
        if (dialogResult == -1 && !urlMode) return;
        else if (dialogResult == -1 && urlMode) {
            this->startNetwork();
            return;
        }
        netInstStuff::installTitleNet(this->selectedUrls, dialogResult, this->alternativeNames, sourceString);
        return;
    }

    void netInstPage::onInput(u64 Down, u64 Up, u64 Held, pu::ui::Touch Pos) {
        if (Down & HidNpadButton_B) {
            if (this->selectedUrls.size() > 0){
                netInstStuff::sendExitCommands(inst::util::formatUrlLink(this->selectedUrls[0]));
            }
            netInstStuff::OnUnwound();
            mainApp->LoadLayout(mainApp->mainPage);
        }
        if ((Down & HidNpadButton_A) || (Up & TouchPseudoKey)) {
            this->selectTitle(this->menu->GetSelectedIndex());
            if (this->menu->GetItems().size() == 1 && this->selectedUrls.size() == 1) {
                this->startInstall(false);
            }
        }
        if ((Down & HidNpadButton_Y)) {
            if (this->selectedUrls.size() == this->menu->GetItems().size()) this->drawMenuItems(true);
            else {
                for (size_t i = 0; i < this->menu->GetItems().size(); i++) {
                    if (this->menu->GetItems()[i]->GetIcon() == "romfs:/images/icons/checkbox-blank-outline.png")
                        this->selectTitle(i);
                }
                this->drawMenuItems(false);
            }
        }
        if (Down & HidNpadButton_Plus) {
            if (this->selectedUrls.size() > 0) {
                this->startInstall(false);
            }
        }
    }
}
