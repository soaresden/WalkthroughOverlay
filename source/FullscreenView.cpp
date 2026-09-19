// FullscreenView.cpp — shared header/footer/progress drawing for the readers.
#include "FullscreenView.hpp"

namespace ui {

    void drawHeader(tsl::gfx::Renderer* r, const std::string& title, const std::string& status) {
        const s32 statusW = status.empty() ? 0 : r->getTextDimensions(status, false, 15).first + 12;
        std::string t = r->limitStringLength(title, false, 20, W - 32 - statusW);
        r->drawString(t, false, 16, 30, 20, r->a(text()));
        if (!status.empty())
            r->drawString(status, false, W - 16 - statusW + 12, 30, 15, r->a(dim()));
        r->drawRect(16, HEADER_H - 1, W - 32, 1, r->a(line()));
    }

    s32 drawFooter(tsl::gfx::Renderer* r, bool showHelp, const std::string& row1, const std::string& row2) {
        const s32 h = showHelp ? FOOTER_H : FOOTER_MIN_H;
        const s32 top = H - h;
        r->drawRect(16, top, W - 32, 1, r->a(line()));
        if (showHelp) {
            r->drawString(row1, false, 16, top + 24, 14, r->a(dim()));
            r->drawString(row2, false, 16, top + 46, 14, r->a(dim()));
        } else {
            r->drawString(" help", false, W - 16 - 52, top + 16, 12, r->a(dim()));
        }
        return top - 4;
    }

    void drawProgress(tsl::gfx::Renderer* r, s32 top, s32 bottom, float fraction, float visibleFraction) {
        const s32 x = W - 6;
        const s32 h = bottom - top;
        if (h <= 0) return;
        r->drawRect(x, top, 3, h, r->a(tsl::Color(0x3, 0x3, 0x3, 0xF)));
        s32 thumbH = (s32)(h * visibleFraction);
        if (thumbH < 8) thumbH = 8;
        if (thumbH > h) thumbH = h;
        s32 thumbY = top + (s32)((h - thumbH) * fraction);
        r->drawRect(x, thumbY, 3, thumbH, r->a(accent()));
    }
}
