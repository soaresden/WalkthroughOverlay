// Config.cpp
#include "Config.hpp"
#include <fstream>
#include <filesystem>

const char* Config::FILE_PATH = "/switch/WalkthroughOverlay/config.json";

nlohmann::json Config::read() {
    try {
        std::ifstream file(FILE_PATH);
        if (!file.is_open()) {
            // Si le fichier n'existe pas, initialiser une nouvelle configuration
            return initialize();
        }
        nlohmann::json config;
        file >> config;
        return config;
    }
    catch (...) {
        // En cas d'erreur, initialiser une nouvelle configuration
        return initialize();
    }
}

void Config::update(std::function<void(nlohmann::json&)> const& updater) {
    nlohmann::json config = read();
    updater(config);
    // Cr�er le r�pertoire si n�cessaire
    std::filesystem::create_directories(std::filesystem::path(FILE_PATH).parent_path());
    // �crire la configuration mise � jour
    std::ofstream file(FILE_PATH);
    file << config.dump(4);  // Indentation de 4 espaces
}

nlohmann::json Config::initialize() {
    nlohmann::json defaultConfig;
    // Configuration par d�faut
    defaultConfig["version"] = "1.0.0";
    defaultConfig["favorites"] = nlohmann::json::array();
    // Cr�er le r�pertoire si n�cessaire
    std::filesystem::create_directories(std::filesystem::path(FILE_PATH).parent_path());
    // �crire la configuration par d�faut
    std::ofstream file(FILE_PATH);
    file << defaultConfig.dump(4);  // Indentation de 4 espaces
    return defaultConfig;
}