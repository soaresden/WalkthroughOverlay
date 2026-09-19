// FullscreenView.hpp — a root element that hands the whole framebuffer to a Gui.
//
// libtesla's OverlayFrame reserves a header and a footer; the readers want every
// pixel, so they use this instead and draw their own chrome.
#pragma once
#include <tesla.hpp>
#include <functional>

class FullscreenView : public tsl::elm::Element {
public:
    explicit FullscreenView(std::function<void(tsl::gfx::Renderer*)> drawFn)
        : Element(), m_draw(std::move(drawFn)) {
        m_isItem = false;
    }

    void draw(tsl::gfx::Renderer* renderer) override { m_draw(renderer); }
    void layout(u16, u16, u16, u16) override {
        setBoundaries(0, 0, tsl::cfg::FramebufferWidth, tsl::cfg::FramebufferHeight);
    }
    Element* requestFocus(Element*, tsl::FocusDirection) override {
        // libtesla turns lone L/R/ZL/ZR presses into "jump/skip" requests for the
        // focused list. Nothing here consumes them, so clear them or the next
        // list we return to would jump on its own.
        jumpToTop.store(false, std::memory_order_release);
        jumpToBottom.store(false, std::memory_order_release);
        skipUp.store(false, std::memory_order_release);
        skipDown.store(false, std::memory_order_release);
        return nullptr;
    }

private:
    std::function<void(tsl::gfx::Renderer*)> m_draw;
};

namespace ui {
    // Shared chrome helpers for the fullscreen readers.
    constexpr s32 W = 448;
    constexpr s32 H = 720;
    constexpr s32 HEADER_H = 44;
    constexpr s32 FOOTER_H = 62;     // with key legend
    constexpr s32 FOOTER_MIN_H = 22; // legend hidden

    inline tsl::Color bg()      { return tsl::Color(0x0, 0x0, 0x0, 0xE); }
    inline tsl::Color text()    { return tsl::Color(0xF, 0xF, 0xF, 0xF); }
    inline tsl::Color dim()     { return tsl::Color(0xA, 0xA, 0xA, 0xF); }
    inline tsl::Color accent()  { return tsl::Color(0x4, 0xB, 0xF, 0xF); }
    inline tsl::Color mark()    { return tsl::Color(0xF, 0x8, 0x2, 0xF); }
    inline tsl::Color line()    { return tsl::Color(0x5, 0x5, 0x5, 0xF); }

    // Header: title (truncated) on the left, status on the right.
    void drawHeader(tsl::gfx::Renderer* r, const std::string& title, const std::string& status);
    // Footer: up to two rows of key hints; returns the y where content must stop.
    s32 drawFooter(tsl::gfx::Renderer* r, bool showHelp, const std::string& row1, const std::string& row2);
    // Thin vertical progress indicator along the right edge of the content area.
    void drawProgress(tsl::gfx::Renderer* r, s32 top, s32 bottom, float fraction, float visibleFraction);
}
