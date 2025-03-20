// Config.hpp
#pragma once
#include "../libs/json/json.hpp"
#include <functional>

class Config {
public:
    static nlohmann::json read();
    static void update(std::function<void(nlohmann::json&)> const& updater);
    static nlohmann::json initialize();
private:
    Config() = default;
    static const char* FILE_PATH;
};