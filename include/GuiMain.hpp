// GuiMain.hpp — landing menu.
#pragma once
#include <tesla.hpp>

class GuiMain : public tsl::Gui {
public:
    tsl::elm::Element* createUI() override;
};

class GuiHelp : public tsl::Gui {
public:
    tsl::elm::Element* createUI() override;
};
