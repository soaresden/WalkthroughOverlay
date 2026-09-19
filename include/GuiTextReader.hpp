// GuiTextReader.hpp — full-screen text guide reader.
//
//  Left stick / D-pad up-down : scroll (hold to accelerate)
//  Left stick / D-pad left-right : page up / page down
//  ZL / ZR         : jump 10 % back / forward
//  L / R           : previous / next bookmark
//  Y               : toggle bookmark on the top line
//  Right stick up/down : font size,  right stick click : reset size
//  X               : monospace <-> proportional font
//  Minus           : show / hide the key legend
//  B               : back (position is remembered)
#pragma once
#include <tesla.hpp>
#include <set>
#include <string>
#include <vector>
#include "TextDocument.hpp"
#include "Input.hpp"

class GuiTextReader : public tsl::Gui {
public:
    explicit GuiTextReader(const std::string& path);
    ~GuiTextReader() override;

    tsl::elm::Element* createUI() override;
    bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touch,
                     HidAnalogStickState left, HidAnalogStickState right) override;

private:
    struct VisualLine {
        u32 logical;     // logical line number
        u16 start, len;  // byte range inside the logical line (a wrapped segment)
        bool first;      // first segment of its logical line
    };

    void draw(tsl::gfx::Renderer* r);
    void ensureOpen();
    void layoutVisible(tsl::gfx::Renderer* r);
    void wrapLine(const std::string& text, u32 logical, u32 maxChars, std::vector<VisualLine>& out) const;
    u32  charsPerLine(tsl::gfx::Renderer* r);

    void scrollLines(s32 delta);
    void scrollTo(u32 line);
    void pageDown();
    void pageUp();
    void toggleBookmark();
    void prevBookmark();
    void nextBookmark();
    void saveResume();

    std::string m_path;
    std::string m_title;
    TextDocument m_doc;
    bool m_opened = false;
    bool m_openFailed = false;
    u8   m_loadFrames = 0;

    u32 m_top = 0;               // first logical line shown
    u32 m_visibleLogical = 1;    // logical lines fully shown last frame
    std::set<u32> m_bookmarks;

    int  m_fontSize;
    bool m_mono;
    bool m_showHelp;
    float m_charW = 8.0f;        // cached average glyph width for (fontSize, mono)
    int  m_charWFor = -1;
    bool m_charWMono = true;

    std::vector<VisualLine> m_visual;

    input::Repeater m_repUp, m_repDown, m_repLeft, m_repRight, m_repZL, m_repZR, m_repSizeUp, m_repSizeDown;
};
