// GuiTextReader.cpp
#include "GuiTextReader.hpp"
#include "FullscreenView.hpp"
#include "Config.hpp"
#include "Files.hpp"
#include <algorithm>
#include <cstdlib>

namespace {
    constexpr s32 TEXT_X      = 16;
    constexpr s32 TEXT_RIGHT  = ui::W - 14; // leave room for the progress bar
    constexpr s32 CONTENT_TOP = ui::HEADER_H + 6;

    inline s32 lineHeight(int fontSize) { return fontSize + fontSize / 4 + 1; }
}

GuiTextReader::GuiTextReader(const std::string& path)
    : m_path(path),
      m_title(files::fileName(path)),
      m_repZL(300, 120), m_repZR(300, 120),
      m_repSizeUp(250, 110, 100000, 110), m_repSizeDown(250, 110, 100000, 110)
{
    auto& cfg = Config::get();
    m_fontSize = cfg.fontSize;
    m_mono     = cfg.monospace;
    m_showHelp = cfg.showHelp;
    m_bookmarks = cfg.getBookmarks(path);
    std::string resume = cfg.getResume(path);
    if (!resume.empty()) m_top = (u32)std::strtoul(resume.c_str(), nullptr, 10);
}

GuiTextReader::~GuiTextReader() {
    saveResume();
}

tsl::elm::Element* GuiTextReader::createUI() {
    return new FullscreenView([this](tsl::gfx::Renderer* r) { draw(r); });
}

// ---------------------------------------------------------------- persistence

void GuiTextReader::saveResume() {
    if (!m_opened || m_openFailed) return;
    auto& cfg = Config::get();
    bool dirty = false;
    if (cfg.fontSize != m_fontSize) { cfg.fontSize = m_fontSize; dirty = true; }
    if (cfg.monospace != m_mono)    { cfg.monospace = m_mono;    dirty = true; }
    if (cfg.showHelp != m_showHelp) { cfg.showHelp = m_showHelp; dirty = true; }
    if (dirty) cfg.save();
    cfg.setResume(m_path, m_top == 0 ? "" : std::to_string(m_top));
}

// ---------------------------------------------------------------- opening

void GuiTextReader::ensureOpen() {
    if (m_opened) return;
    m_opened = true;
    if (!m_doc.open(m_path)) { m_openFailed = true; return; }
    if (m_doc.lineCount() == 0) m_top = 0;
    else if (m_top >= m_doc.lineCount()) m_top = m_doc.lineCount() - 1;
}

// ---------------------------------------------------------------- wrapping

u32 GuiTextReader::charsPerLine(tsl::gfx::Renderer* r) {
    if (m_charWFor != m_fontSize || m_charWMono != m_mono) {
        // Measure a representative sample once per (size, font) combination.
        static const std::string sample =
            "The quick brown fox jumps over the lazy dog 0123456789 ITEMS: HP/MP -> [x] ";
        s32 w = r->getTextDimensions(sample, m_mono, (u32)m_fontSize).first;
        m_charW = std::max(1.0f, (float)w / (float)sample.size());
        if (!m_mono) m_charW *= 1.06f; // proportional estimate: keep a little slack
        m_charWFor = m_fontSize;
        m_charWMono = m_mono;
    }
    u32 n = (u32)((TEXT_RIGHT - TEXT_X) / m_charW);
    return n < 8 ? 8 : n;
}

void GuiTextReader::wrapLine(const std::string& text, u32 logical, u32 maxChars, std::vector<VisualLine>& out) const {
    if (text.empty()) { out.push_back({ logical, 0, 0, true }); return; }

    size_t start = 0;
    bool first = true;
    const size_t n = text.size();
    while (start < n) {
        // walk maxChars code points, remembering the last break opportunity
        size_t i = start, chars = 0, lastSpace = std::string::npos;
        while (i < n && chars < maxChars) {
            unsigned char c = text[i];
            if (c == ' ') lastSpace = i;
            // advance one code point
            ++i;
            while (i < n && ((unsigned char)text[i] & 0xC0) == 0x80) ++i;
            ++chars;
        }
        size_t end = i;
        if (i < n) {
            // need to break: prefer the last space unless it is too far back
            if (lastSpace != std::string::npos && lastSpace > start && (i - lastSpace) < maxChars / 2)
                end = lastSpace;
        }
        if (end <= start) end = i; // safety
        out.push_back({ logical, (u16)start, (u16)std::min<size_t>(end - start, 0xFFFF), first });
        first = false;
        start = end;
        while (start < n && text[start] == ' ') ++start; // swallow the break space(s)
    }
}

void GuiTextReader::layoutVisible(tsl::gfx::Renderer* r) {
    m_visual.clear();
    const s32 lh = lineHeight(m_fontSize);
    const s32 bottom = ui::H - (m_showHelp ? ui::FOOTER_H : ui::FOOTER_MIN_H) - 4;
    const u32 rows = (u32)std::max<s32>(1, (bottom - CONTENT_TOP) / lh);
    const u32 maxChars = charsPerLine(r);
    const u32 total = m_doc.lineCount();

    std::vector<VisualLine> tmp;
    u32 logical = m_top;
    u32 fullyVisible = 0;
    while (m_visual.size() < rows && logical < total) {
        tmp.clear();
        wrapLine(m_doc.line(logical), logical, maxChars, tmp);
        bool fits = (m_visual.size() + tmp.size()) <= rows;
        for (auto& v : tmp) {
            if (m_visual.size() >= rows) break;
            m_visual.push_back(v);
        }
        if (fits) ++fullyVisible;
        ++logical;
    }
    m_visibleLogical = std::max<u32>(1, fullyVisible);
}

// ---------------------------------------------------------------- drawing

void GuiTextReader::draw(tsl::gfx::Renderer* r) {
    r->fillScreen(r->a(ui::bg()));

    if (!m_opened) {
        ui::drawHeader(r, m_title, "");
        r->drawString("Loading…", false, TEXT_X, CONTENT_TOP + 40, 18, r->a(ui::dim()));
        // Open on the second frame so the "Loading" text is visible while scanning big files.
        if (m_loadFrames++ >= 1) ensureOpen();
        return;
    }
    if (m_openFailed) {
        ui::drawHeader(r, m_title, "");
        r->drawString("Could not open file", false, TEXT_X, CONTENT_TOP + 40, 18, r->a(ui::mark()));
        r->drawString(m_path, false, TEXT_X, CONTENT_TOP + 70, 13, r->a(ui::dim()));
        ui::drawFooter(r, true, " Back", "");
        return;
    }

    const u32 total = m_doc.lineCount();
    layoutVisible(r);

    // header
    char status[48];
    const u32 pct = total ? (u32)((u64)(m_top + m_visibleLogical) * 100 / total) : 100;
    snprintf(status, sizeof(status), "%u%%  %u/%u", pct > 100 ? 100 : pct, m_top + 1, total);
    ui::drawHeader(r, m_title, status);

    // body
    const s32 lh = lineHeight(m_fontSize);
    s32 y = CONTENT_TOP;
    const auto textColor = r->a(ui::text());
    for (auto& v : m_visual) {
        const bool marked = m_bookmarks.count(v.logical) != 0;
        if (marked) {
            r->drawRect(TEXT_X - 10, y, 4, lh, r->a(ui::mark()));
            r->drawRect(TEXT_X - 4, y, TEXT_RIGHT - TEXT_X + 4, lh, r->a(tsl::Color(0x4, 0x2, 0x0, 0x8)));
        }
        if (v.len) {
            const std::string& full = m_doc.line(v.logical);
            if (v.start < full.size()) {
                std::string seg = full.substr(v.start, v.len);
                r->drawString(seg, m_mono, TEXT_X, y + m_fontSize, (u32)m_fontSize, textColor);
            }
        }
        y += lh;
    }

    // footer + progress
    char sizeBuf[24];
    snprintf(sizeBuf, sizeof(sizeBuf), " Size %d", m_fontSize);
    const s32 bottom = ui::drawFooter(r, m_showHelp,
        " Scroll    ±10%    Bookmarks    Mark",
        std::string(sizeBuf) + "    " + (m_mono ? "Mono" : "Prop") + "    Legend    Back");
    if (total) {
        ui::drawProgress(r, CONTENT_TOP, bottom,
            total > m_visibleLogical ? (float)m_top / (float)(total - m_visibleLogical) : 0.0f,
            (float)m_visibleLogical / (float)total);
    }
}

// ---------------------------------------------------------------- navigation

void GuiTextReader::scrollTo(u32 line) {
    const u32 total = m_doc.lineCount();
    if (total == 0) { m_top = 0; return; }
    u32 maxTop = total > m_visibleLogical ? total - m_visibleLogical : 0;
    m_top = std::min(line, maxTop);
}

void GuiTextReader::scrollLines(s32 delta) {
    s64 t = (s64)m_top + delta;
    if (t < 0) t = 0;
    scrollTo((u32)t);
}

void GuiTextReader::pageDown() { scrollLines((s32)std::max<u32>(1, m_visibleLogical - 1)); }
void GuiTextReader::pageUp()   { scrollLines(-(s32)std::max<u32>(1, m_visibleLogical - 1)); }

void GuiTextReader::toggleBookmark() {
    if (!m_bookmarks.erase(m_top)) m_bookmarks.insert(m_top);
    Config::get().setBookmarks(m_path, m_bookmarks);
}

void GuiTextReader::prevBookmark() {
    auto it = m_bookmarks.lower_bound(m_top);
    if (it == m_bookmarks.begin()) return;
    --it;
    scrollTo(*it);
}

void GuiTextReader::nextBookmark() {
    auto it = m_bookmarks.upper_bound(m_top);
    if (it == m_bookmarks.end()) return;
    scrollTo(*it);
}

// ---------------------------------------------------------------- input

bool GuiTextReader::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState&, HidAnalogStickState, HidAnalogStickState) {
    if (keysDown & HidNpadButton_B) {
        tsl::goBack(); // the destructor saves the position
        return true;
    }
    if (!m_opened || m_openFailed) return false;

    const u32 total = m_doc.lineCount();
    const u32 tenPct = std::max<u32>(1, total / 10);

    for (u32 i = m_repDown.update(keysHeld & input::NAV_DOWN); i; --i) scrollLines(1);
    for (u32 i = m_repUp.update(keysHeld & input::NAV_UP); i; --i)     scrollLines(-1);
    for (u32 i = m_repRight.update(keysHeld & input::NAV_RIGHT); i; --i) pageDown();
    for (u32 i = m_repLeft.update(keysHeld & input::NAV_LEFT); i; --i)   pageUp();
    for (u32 i = m_repZR.update(keysHeld & HidNpadButton_ZR); i; --i) scrollLines((s32)tenPct);
    for (u32 i = m_repZL.update(keysHeld & HidNpadButton_ZL); i; --i) scrollLines(-(s32)tenPct);

    for (u32 i = m_repSizeUp.update(keysHeld & HidNpadButton_StickRUp); i; --i)
        m_fontSize = std::min(m_fontSize + 1, 40);
    for (u32 i = m_repSizeDown.update(keysHeld & HidNpadButton_StickRDown); i; --i)
        m_fontSize = std::max(m_fontSize - 1, 10);
    if (keysDown & HidNpadButton_StickR) m_fontSize = 16;

    if (keysDown & HidNpadButton_Y) toggleBookmark();
    if (keysDown & HidNpadButton_L) prevBookmark();
    if (keysDown & HidNpadButton_R) nextBookmark();
    if (keysDown & HidNpadButton_X) m_mono = !m_mono;
    if (keysDown & HidNpadButton_Minus) m_showHelp = !m_showHelp;

    return true;
}
