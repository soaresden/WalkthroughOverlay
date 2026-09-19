// GuiMain.cpp
#include "GuiMain.hpp"
#include "GuiBrowser.hpp"
#include "Config.hpp"
#include "Memory.hpp"
#include "Paths.hpp"

namespace {
    tsl::elm::ListItem* menuItem(const std::string& text, const std::string& value, std::function<void()> action) {
        auto* item = new tsl::elm::ListItem(text, value);
        item->setClickListener([action](u64 keys) {
            if (keys & HidNpadButton_A) { action(); return true; }
            return false;
        });
        return item;
    }
}

tsl::elm::Element* GuiMain::createUI() {
    auto* frame = new tsl::elm::OverlayFrame("Walkthrough", APP_VERSION);
    auto* list = new tsl::elm::List();

    list->addItem(menuItem("Guides", "", [] { tsl::changeTo<GuiBrowser>(paths::GUIDES); }));
    list->addItem(menuItem("Favorites", "", [] { tsl::changeTo<GuiFavorites>(); }));
    list->addItem(menuItem("Browse SD card", "", [] { tsl::changeTo<GuiBrowser>(paths::SD_ROOT); }));
    list->addItem(menuItem("Help", "", [] { tsl::changeTo<GuiHelp>(); }));

    list->addItem(new tsl::elm::CategoryHeader("Overlay memory"));
    list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        const mem::Info i = mem::info();
        const float frac = i.heapTotal ? (float)i.heapUsed / (float)i.heapTotal : 0.f;
        // bar
        const s32 bx = x + 4, by = y + 14, bw = w - 8, bh = 10;
        r->drawRect(bx, by, bw, bh, r->a(tsl::Color(0x3, 0x3, 0x3, 0xF)));
        tsl::Color fill = frac < 0.6f ? tsl::Color(0x4, 0xC, 0x6, 0xF)
                        : frac < 0.85f ? tsl::Color(0xE, 0xB, 0x3, 0xF) : tsl::Color(0xE, 0x4, 0x3, 0xF);
        r->drawRect(bx, by, (s32)(bw * frac), bh, r->a(fill));
        // text
        r->drawString("Used " + mem::summary(), false, bx, by + 32, 15, r->a(tsl::defaultTextColor));
        r->drawString(mem::prettyMB(i.heapFree) + " MB free for images", false, bx, by + 54, 13, r->a(tsl::Color(0xA, 0xA, 0xA, 0xF)));
        r->drawString("Bigger heap: Ultrahand > Settings > Memory expansion", false, bx, by + 74, 12, r->a(tsl::Color(0x8, 0x8, 0x8, 0xF)));
    }), 100);

    frame->setContent(list);
    return frame;
}

tsl::elm::Element* GuiHelp::createUI() {
    auto* frame = new tsl::elm::OverlayFrame("Help", "Controls");
    auto* list = new tsl::elm::List();

    struct Row { const char* keys; const char* what; };
    auto section = [&](const char* title, std::initializer_list<Row> rows) {
        list->addItem(new tsl::elm::CategoryHeader(title));
        for (auto& row : rows) {
            std::string keys = row.keys, what = row.what;
            list->addItem(new tsl::elm::CustomDrawer([keys, what](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
                r->drawString(keys, false, x + 4, y + 20, 16, r->a(tsl::defaultTextColor));
                r->drawString(what, false, x + 110, y + 20, 15, r->a(tsl::Color(0xC, 0xC, 0xC, 0xF)));
            }), 28);
        }
    };

    section("Lists", {
        { "", "Open" },
        { "", "Star / unstar (Favorites)" },
        { "", "Back" },
    });
    section("Text reader", {
        { " ↑↓", "Scroll (hold to speed up)" },
        { " ←→", "Page up / down" },
        { " ", "Jump 10 % back / forward" },
        { "", "Bookmark the top line" },
        { " ", "Previous / next bookmark" },
        { " ↑↓", "Font size" },
        { "", "Reset font size" },
        { "", "Monospace / proportional" },
        { "", "Show / hide legend" },
    });
    section("Image viewer", {
        { "", "Pan" },
        { " ↑↓", "Zoom in / out" },
        { "", "Back to fit-width" },
        { "", "Fit width / fit page" },
        { " ", "Previous / next page" },
        { " ", "10 pages back / forward" },
    });
    section("Files", {
        { "guides/", "sd:/switch/WalkthroughOverlay/guides/" },
        { "Text", ".txt  .md  .log  .nfo" },
        { "Images", ".png  .jpg  (a folder = a document)" },
        { "PDF", "Convert on PC with tools/guide2switch.py" },
    });

    frame->setContent(list);
    return frame;
}
