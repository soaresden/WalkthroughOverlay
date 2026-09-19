// Paths.hpp — every on-SD location the overlay touches, in one place.
#pragma once
#include <string>

namespace paths {
    inline const std::string SD_ROOT     = "sdmc:/";
    inline const std::string APP_ROOT    = "sdmc:/switch/WalkthroughOverlay/";
    inline const std::string GUIDES      = "sdmc:/switch/WalkthroughOverlay/guides/";
    inline const std::string CONFIG_DIR  = "sdmc:/config/WalkthroughOverlay/";
    inline const std::string CONFIG_FILE = "sdmc:/config/WalkthroughOverlay/config.ini";
}

#ifndef APP_VERSION
#define APP_VERSION "2.0.0"
#endif
