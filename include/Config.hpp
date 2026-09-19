// Config.hpp — persistent settings, favorites, resume positions and bookmarks.
//
// Stored as a tiny INI-like file (see Paths.hpp). Sections:
//   [settings]   key=value
//   [favorites]  <path>|1
//   [resume]     <path>|<line number or last page file name>
//   [bookmarks]  <path>|1,42,1337
// '|' separates path keys from values because it can never appear in a FAT path.
#pragma once
#include <string>
#include <map>
#include <set>
#include <vector>
#include <switch.h>

class Config {
public:
    static Config& get();

    void load();
    void save();

    // --- reader settings ---
    int  fontSize   = 16;    // text reader font size (px)
    bool monospace  = true;  // keep ASCII art aligned by default
    bool showHelp   = true;  // draw the key legend at the bottom of readers

    // --- favorites ---
    bool isFavorite(const std::string& path) const;
    void toggleFavorite(const std::string& path);
    std::vector<std::string> favorites() const;

    // --- resume ---
    std::string getResume(const std::string& path) const;
    void setResume(const std::string& path, const std::string& value);

    // --- bookmarks (text files: logical line numbers) ---
    std::set<u32> getBookmarks(const std::string& path) const;
    void setBookmarks(const std::string& path, const std::set<u32>& marks);

private:
    Config() = default;
    bool m_loaded = false;
    std::set<std::string> m_favorites;
    std::map<std::string, std::string> m_resume;
    std::map<std::string, std::string> m_bookmarks;
};
