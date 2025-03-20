// HelpMenu.hpp

#pragma once

#include <tesla.hpp>

class HelpMenu : public tsl::Gui {
public:
    tsl::elm::Element* createUI() override;

private:
    void showKeybind(tsl::gfx::Renderer* renderer, u16 offset, u16 i, const char* keys, const char* desc);
};
