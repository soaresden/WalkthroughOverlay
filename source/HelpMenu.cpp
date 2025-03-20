//HelpMenu.cpp
// Key Bindings Mapping
// ==========================
// "\uE081"  -> Left Stick Icon
// "\uE082"  -> Right Stick Icon
// 
// "\uE08A"  -> Left Stick Click Icon
// "\uE08B"  -> Right Stick Click Icon
// 
// "\uE090" --> Right Arrow
// "\uE091" --> Left Arrow
// "\uE092" --> Up Arrow
// "\uE093" --> Down Arrow
// 
// "\uE07D"  -> Dpad UpDown
// 
// 
// "\uE085" --> ZL Icon
// "\uE086" --> ZR Icon
// 
// "\uE0A0"  -> B icon
// "\uE0A1"  -> A icon
// "\uE0A2"  -> X icon
// "\uE0A3"  -> Y Icon
// "\uE0A4"  -> L Icon
// "\uE0A5"  -> R Icon
// "\uE0A6"  -> ZL Icon
// "\uE0A7"  -> ZR Icon
// "\uE0A8"  -> SL Icon
// "\uE0A9"  -> SR Icon


// HelpMenu.cpp

#include "HelpMenu.hpp"
#include "Overlay.hpp"

tsl::elm::Element* HelpMenu::createUI() {
    auto* frame = new tsl::elm::OverlayFrame("", "");

    frame->setContent(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* renderer, u16 x, u16 y, u16 w, u16 h) {
        u16 offset = 150, i = 0;
        renderer->drawString("Menu", false, 50, offset, 30, 0xFFFF);
        showKeybind(renderer, offset, i++, "\uE0A3", "Toggle Favorite");

        offset = 250, i = 0;
        renderer->drawString("Txt Controls", false, 50, offset, 30, 0xFFFF);
        showKeybind(renderer, offset, i++, "", "Written in Menu");
        }));

    return frame;
}

void HelpMenu::showKeybind(tsl::gfx::Renderer* renderer, u16 offset, u16 i, const char* keys, const char* desc) {
    renderer->drawString(keys, false, 50, offset + 50 + 32 * i, 24, 0xFFFF);
    renderer->drawString(desc, false, 190, offset + 50 + 32 * i, 24, 0xFFFF);
}
