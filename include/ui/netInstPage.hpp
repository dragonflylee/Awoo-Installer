#pragma once
#include <pu/Plutonium>
#include "util/drive.hpp"

using namespace pu::ui::elm;
namespace inst::ui {
    class netInstPage : public pu::ui::Layout
    {
        public:
            netInstPage();
            PU_SMART_CTOR(netInstPage)
            void startInstall(bool urlMode);
            void startNetwork();
            void onInput(u64 Down, u64 Up, u64 Held, pu::ui::Touch Pos);
            TextBlock::Ref pageInfoText;
            Image::Ref titleImage;
            TextBlock::Ref appVersionText;
        private:
            drive::drive::ref client;
            drive::drive::entries ourUrls;
            std::vector<std::string> selectedUrls;
            std::vector<std::string> lastFileId;
            std::vector<std::string> alternativeNames;
            TextBlock::Ref butText;
            Rectangle::Ref topRect;
            Rectangle::Ref infoRect;
            Rectangle::Ref botRect;
            pu::ui::elm::Menu::Ref menu;
            Image::Ref infoImage;
            Image::Ref qrcodeImage;
            void drawMenuItems(bool clearItems);
            void selectTitle(int selectedIndex);
    };
}