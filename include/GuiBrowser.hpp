// GuiBrowser.hpp — folder listing (also used for the Favorites list).
#pragma once
#include <tesla.hpp>
#include <string>
#include <vector>
#include "Files.hpp"

class GuiBrowser : public tsl::Gui {
public:
    // Browse a directory.
    explicit GuiBrowser(const std::string& dir);
    // Show an explicit list of entries under a title (favorites).
    GuiBrowser(const std::string& title, std::vector<files::Entry> entries);

    tsl::elm::Element* createUI() override;

    static void openEntry(const files::Entry& e);
    static std::string labelFor(const files::Entry& e);

private:
    std::string m_dir;     // empty for a custom list
    std::string m_title;
    std::vector<files::Entry> m_entries;
    bool m_customList = false;
};

class GuiFavorites : public GuiBrowser {
public:
    GuiFavorites();
};
