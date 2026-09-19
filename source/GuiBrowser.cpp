// GuiBrowser.cpp
#include "GuiBrowser.hpp"
#include "GuiTextReader.hpp"
#include "GuiImageViewer.hpp"
#include "Config.hpp"
#include "Paths.hpp"

namespace {
    constexpr const char* STAR = "★";

    std::string shortPath(const std::string& p) {
        // "sdmc:/switch/WalkthroughOverlay/guides/Zelda/" -> "guides/Zelda/"
        if (p.rfind(paths::GUIDES, 0) == 0) return "guides/" + p.substr(paths::GUIDES.size());
        if (p.rfind("sdmc:/", 0) == 0) return p.substr(6).empty() ? "sdmc:/" : p.substr(6);
        return p;
    }
}

GuiBrowser::GuiBrowser(const std::string& dir)
    : m_dir(dir), m_title(dir == paths::SD_ROOT ? "SD card" : files::fileName(dir))
{
    if (dir == paths::GUIDES && !files::exists(dir)) files::mkdirs(dir);
    m_entries = files::list(dir);
}

GuiBrowser::GuiBrowser(const std::string& title, std::vector<files::Entry> entries)
    : m_title(title), m_entries(std::move(entries)), m_customList(true) {}

std::string GuiBrowser::labelFor(const files::Entry& e) {
    return e.name;
}

void GuiBrowser::openEntry(const files::Entry& e) {
    switch (e.kind) {
        case files::Kind::Dir:   tsl::changeTo<GuiBrowser>(e.path); break;
        case files::Kind::Text:  tsl::changeTo<GuiTextReader>(e.path); break;
        case files::Kind::Image: tsl::changeTo<GuiImageViewer>(e.path); break;
        default: break;
    }
}

tsl::elm::Element* GuiBrowser::createUI() {
    auto* frame = new tsl::elm::OverlayFrame(m_title, m_customList ? "" : shortPath(m_dir));
    auto* list = new tsl::elm::List();
    auto& cfg = Config::get();

    // "Continue where you left off" for image documents
    if (!m_customList) {
        const std::string resume = cfg.getResume(m_dir);
        if (!resume.empty() && files::isImage(resume) && files::exists(m_dir + resume)) {
            auto* cont = new tsl::elm::ListItem("▶ Continue", resume);
            const std::string full = m_dir + resume;
            cont->setClickListener([full](u64 keys) {
                if (keys & HidNpadButton_A) { tsl::changeTo<GuiImageViewer>(full); return true; }
                return false;
            });
            list->addItem(cont);
        }
    }

    if (m_entries.empty()) {
        list->addItem(new tsl::elm::CategoryHeader(m_customList ? "No favorites yet" : "Nothing here"));
        const std::string hint = m_customList
            ? "Press  on any file or folder to star it."
            : (m_dir == paths::GUIDES
                ? "Copy .txt guides or folders of .png/.jpg pages\nto sd:/switch/WalkthroughOverlay/guides/"
                : "No .txt / .md / .png / .jpg files in this folder.");
        list->addItem(new tsl::elm::CustomDrawer([hint](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
            size_t nl = hint.find('\n');
            r->drawString(hint.substr(0, nl), false, x + 4, y + 22, 15, r->a(tsl::defaultTextColor));
            if (nl != std::string::npos)
                r->drawString(hint.substr(nl + 1), false, x + 4, y + 46, 15, r->a(tsl::defaultTextColor));
        }), 60);
    }

    for (auto& e : m_entries) {
        auto* item = new tsl::elm::ListItem(labelFor(e), cfg.isFavorite(e.path) ? STAR : "");
        const files::Entry entry = e;
        item->setClickListener([entry, item](u64 keys) {
            if (keys & HidNpadButton_A) { openEntry(entry); return true; }
            if (keys & HidNpadButton_Y) {
                auto& c = Config::get();
                c.toggleFavorite(entry.path);
                item->setValue(c.isFavorite(entry.path) ? STAR : "");
                return true;
            }
            return false;
        });
        list->addItem(item);
    }

    frame->setContent(list);
    return frame;
}

// ---------------------------------------------------------------- favorites

static std::vector<files::Entry> favoriteEntries() {
    std::vector<files::Entry> out;
    for (auto& p : Config::get().favorites()) {
        files::Entry e;
        e.path = p;
        e.kind = files::kindOf(p);
        if (e.kind == files::Kind::Dir) {
            std::string parent = files::parentDir(p);
            e.name = files::fileName(p) + "/";
            if (parent != paths::GUIDES && !parent.empty()) e.name = files::fileName(parent) + "/" + e.name;
        } else {
            e.name = files::fileName(p);
        }
        if (!files::exists(p)) e.name = "(missing) " + e.name;
        out.push_back(e);
    }
    return out;
}

GuiFavorites::GuiFavorites() : GuiBrowser("Favorites", favoriteEntries()) {}
