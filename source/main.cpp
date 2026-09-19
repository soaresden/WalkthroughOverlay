// main.cpp — Walkthrough Overlay entry point (libultrahand).
#define TESLA_INIT_IMPL
#include <exception_wrap.hpp>
#include <tesla.hpp>
#include "GuiMain.hpp"
#include "Files.hpp"
#include "Paths.hpp"

class WalkthroughOverlay : public tsl::Overlay {
public:
    // libultrahand already mounts the SD card and brings up the services it needs.
    void initServices() override {
        files::mkdirs(paths::GUIDES);
        files::mkdirs(paths::CONFIG_DIR);
    }
    void exitServices() override {}

    std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<GuiMain>();
    }
};

int main(int argc, char** argv) {
    return tsl::loop<WalkthroughOverlay>(argc, argv);
}
