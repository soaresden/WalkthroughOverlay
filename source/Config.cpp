// Config.cpp
#include "Config.hpp"
#include "Paths.hpp"
#include "Files.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
    void trimCR(std::string& s) {
        while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '\t'))
            s.pop_back();
    }
}

Config& Config::get() {
    static Config instance;
    if (!instance.m_loaded) instance.load();
    return instance;
}

void Config::load() {
    m_loaded = true;
    m_favorites.clear();
    m_resume.clear();
    m_bookmarks.clear();

    FILE* f = fopen(paths::CONFIG_FILE.c_str(), "r");
    if (!f) return;

    char buf[1024];
    std::string section;
    while (fgets(buf, sizeof(buf), f)) {
        std::string line(buf);
        trimCR(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            continue;
        }
        if (section == "settings") {
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq), val = line.substr(eq + 1);
            if (key == "font_size") fontSize = std::atoi(val.c_str());
            else if (key == "monospace") monospace = (val == "1");
            else if (key == "show_help") showHelp = (val == "1");
        } else {
            auto sep = line.find('|');
            if (sep == std::string::npos) continue;
            std::string key = line.substr(0, sep), val = line.substr(sep + 1);
            if (section == "favorites")      m_favorites.insert(key);
            else if (section == "resume")    m_resume[key] = val;
            else if (section == "bookmarks") m_bookmarks[key] = val;
        }
    }
    fclose(f);

    if (fontSize < 10) fontSize = 10;
    if (fontSize > 40) fontSize = 40;
}

void Config::save() {
    files::mkdirs(paths::CONFIG_DIR);
    FILE* f = fopen(paths::CONFIG_FILE.c_str(), "w");
    if (!f) return;

    fprintf(f, "[settings]\nfont_size=%d\nmonospace=%d\nshow_help=%d\n\n",
            fontSize, monospace ? 1 : 0, showHelp ? 1 : 0);

    fprintf(f, "[favorites]\n");
    for (auto& p : m_favorites) fprintf(f, "%s|1\n", p.c_str());

    fprintf(f, "\n[resume]\n");
    for (auto& kv : m_resume) fprintf(f, "%s|%s\n", kv.first.c_str(), kv.second.c_str());

    fprintf(f, "\n[bookmarks]\n");
    for (auto& kv : m_bookmarks)
        if (!kv.second.empty()) fprintf(f, "%s|%s\n", kv.first.c_str(), kv.second.c_str());

    fclose(f);
}

// --- favorites ---
bool Config::isFavorite(const std::string& path) const {
    return m_favorites.count(path) != 0;
}

void Config::toggleFavorite(const std::string& path) {
    if (!m_favorites.erase(path)) m_favorites.insert(path);
    save();
}

std::vector<std::string> Config::favorites() const {
    return std::vector<std::string>(m_favorites.begin(), m_favorites.end());
}

// --- resume ---
std::string Config::getResume(const std::string& path) const {
    auto it = m_resume.find(path);
    return it == m_resume.end() ? "" : it->second;
}

void Config::setResume(const std::string& path, const std::string& value) {
    if (value.empty()) m_resume.erase(path);
    else m_resume[path] = value;
    save();
}

// --- bookmarks ---
std::set<u32> Config::getBookmarks(const std::string& path) const {
    std::set<u32> out;
    auto it = m_bookmarks.find(path);
    if (it == m_bookmarks.end()) return out;
    const char* s = it->second.c_str();
    while (*s) {
        char* end;
        long v = strtol(s, &end, 10);
        if (end == s) break;
        if (v >= 0) out.insert((u32)v);
        s = (*end == ',') ? end + 1 : end;
    }
    return out;
}

void Config::setBookmarks(const std::string& path, const std::set<u32>& marks) {
    std::string v;
    for (u32 m : marks) {
        if (!v.empty()) v += ',';
        v += std::to_string(m);
    }
    if (v.empty()) m_bookmarks.erase(path);
    else m_bookmarks[path] = v;
    save();
}
