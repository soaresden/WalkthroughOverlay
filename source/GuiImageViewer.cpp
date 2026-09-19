// GuiImageViewer.cpp
#include "GuiImageViewer.hpp"
#include "FullscreenView.hpp"
#include "Config.hpp"
#include "Files.hpp"
#include "Memory.hpp"
#include <algorithm>
#include <cmath>

namespace {
    constexpr s32 CONTENT_TOP = ui::HEADER_H + 2;
    constexpr float MIN_SCALE = 0.15f;
    constexpr float MAX_SCALE = 6.0f;
    constexpr float PAN_SPEED = 7.0f;   // screen px per frame while held
    constexpr float ZOOM_STEP = 1.035f; // per frame while held

    // Decode target: up to 2× the screen width so zooming in still shows detail,
    // as tall as the memory budget allows.
    constexpr u32 MAX_DECODE_W = ui::W * 2;
    constexpr u32 MAX_DECODE_H = 4096;
}

GuiImageViewer::GuiImageViewer(const std::string& path) {
    m_showHelp = Config::get().showHelp;
    m_dir = files::parentDir(path);
    m_pages = files::imagesIn(m_dir);
    if (m_pages.empty()) m_pages.push_back(path);
    auto it = std::find(m_pages.begin(), m_pages.end(), path);
    m_index = (it == m_pages.end()) ? 0 : (s32)(it - m_pages.begin());
}

GuiImageViewer::~GuiImageViewer() {
    saveResume();
}

tsl::elm::Element* GuiImageViewer::createUI() {
    return new FullscreenView([this](tsl::gfx::Renderer* r) { draw(r); });
}

s32 GuiImageViewer::contentTop() const { return CONTENT_TOP; }
s32 GuiImageViewer::contentBottom() const {
    return ui::H - (m_showHelp ? ui::FOOTER_H : ui::FOOTER_MIN_H) - 4;
}

// ---------------------------------------------------------------- persistence

void GuiImageViewer::saveResume() {
    auto& cfg = Config::get();
    if (cfg.showHelp != m_showHelp) { cfg.showHelp = m_showHelp; cfg.save(); }
    if (m_pages.size() > 1)
        cfg.setResume(m_dir, files::fileName(m_pages[m_index]));
}

// ---------------------------------------------------------------- loading

void GuiImageViewer::loadCurrent() {
    m_img.release();
    const u64 budget = mem::budget();
    m_loadInfo = mem::prettyMB(budget) + " MB available";
    const bool ok = image::load(m_pages[m_index], MAX_DECODE_W, MAX_DECODE_H, budget, m_img);
    m_state = ok ? State::Ready : State::Failed;
    if (ok) { if (m_fitWhole) fitPage(); else fitWidth(); }
}

void GuiImageViewer::goToPage(s32 index) {
    if (m_pages.empty()) return;
    index = std::clamp<s32>(index, 0, (s32)m_pages.size() - 1);
    if (index == m_index && m_state == State::Ready) return;
    m_index = index;
    m_state = State::NeedLoad;
    m_loadFrames = 0;
}

// ---------------------------------------------------------------- view maths

void GuiImageViewer::fitWidth() {
    if (!m_img.ok()) return;
    m_scale = (float)ui::W / (float)m_img.w;
    m_ox = 0;
    m_oy = (float)contentTop();
    clampView();
}

void GuiImageViewer::fitPage() {
    if (!m_img.ok()) return;
    const float sw = (float)ui::W / (float)m_img.w;
    const float sh = (float)(contentBottom() - contentTop()) / (float)m_img.h;
    m_scale = std::min(sw, sh);
    m_ox = 0;
    m_oy = (float)contentTop();
    clampView();
}

void GuiImageViewer::zoomBy(float k) {
    if (!m_img.ok()) return;
    const float ns = std::clamp(m_scale * k, MIN_SCALE, MAX_SCALE);
    k = ns / m_scale;
    // zoom around the centre of the content area
    const float cx = ui::W / 2.0f;
    const float cy = (contentTop() + contentBottom()) / 2.0f;
    m_ox = cx - (cx - m_ox) * k;
    m_oy = cy - (cy - m_oy) * k;
    m_scale = ns;
    clampView();
}

void GuiImageViewer::clampView() {
    if (!m_img.ok()) return;
    const float sw = m_img.w * m_scale, sh = m_img.h * m_scale;
    const float top = (float)contentTop(), bottom = (float)contentBottom();
    const float areaH = bottom - top;

    if (sw <= ui::W) m_ox = (ui::W - sw) / 2.0f;
    else m_ox = std::clamp(m_ox, (float)ui::W - sw, 0.0f);

    if (sh <= areaH) m_oy = top + (areaH - sh) / 2.0f;
    else m_oy = std::clamp(m_oy, bottom - sh, top);
}

// ---------------------------------------------------------------- drawing

void GuiImageViewer::drawImage(tsl::gfx::Renderer* r, s32 top, s32 bottom) {
    const float inv = 1.0f / m_scale;
    const s32 x0 = std::max<s32>(0, (s32)std::floor(m_ox));
    const s32 x1 = std::min<s32>(ui::W, (s32)std::ceil(m_ox + m_img.w * m_scale));
    const s32 y0 = std::max<s32>(top, (s32)std::floor(m_oy));
    const s32 y1 = std::min<s32>(bottom, (s32)std::ceil(m_oy + m_img.h * m_scale));
    if (x1 <= x0 || y1 <= y0) return;

    // per-column source x lookup, computed once per frame
    static u16 colMap[ui::W];
    for (s32 x = x0; x < x1; ++x) {
        s32 sx = (s32)((x - m_ox) * inv);
        colMap[x] = (u16)std::clamp<s32>(sx, 0, (s32)m_img.w - 1);
    }

    for (s32 y = y0; y < y1; ++y) {
        s32 sy = (s32)((y - m_oy) * inv);
        sy = std::clamp<s32>(sy, 0, (s32)m_img.h - 1);
        const u16* srcRow = m_img.px + (size_t)sy * m_img.w;
        for (s32 x = x0; x < x1; ++x)
            r->setPixel((u32)x, (u32)y, r->a(tsl::Color(srcRow[colMap[x]])));
    }
}

void GuiImageViewer::draw(tsl::gfx::Renderer* r) {
    r->fillScreen(r->a(ui::bg()));

    const std::string name = files::fileName(m_pages[m_index]);
    char pageBuf[32];
    if (m_pages.size() > 1) snprintf(pageBuf, sizeof(pageBuf), "%d/%d", m_index + 1, (int)m_pages.size());
    else pageBuf[0] = 0;

    if (m_state == State::NeedLoad || m_state == State::Loading) {
        ui::drawHeader(r, name, pageBuf);
        r->drawString("Loading…", false, 16, CONTENT_TOP + 40, 18, r->a(ui::dim()));
        r->drawString(mem::prettyMB(mem::budget()) + " MB available", false, 16, CONTENT_TOP + 66, 13, r->a(ui::dim()));
        ui::drawFooter(r, m_showHelp, "", "");
        m_state = State::Loading;
        if (m_loadFrames++ >= 1) loadCurrent();
        return;
    }

    if (m_state == State::Failed) {
        ui::drawHeader(r, name, pageBuf);
        r->drawString("Could not load image", false, 16, CONTENT_TOP + 40, 18, r->a(ui::mark()));
        r->drawString(m_img.error, false, 16, CONTENT_TOP + 66, 14, r->a(ui::text()));
        if (m_img.srcW) {
            char dim[64];
            snprintf(dim, sizeof(dim), "Source: %ux%u   %s", m_img.srcW, m_img.srcH, m_loadInfo.c_str());
            r->drawString(dim, false, 16, CONTENT_TOP + 90, 13, r->a(ui::dim()));
        }
        ui::drawFooter(r, true, " Page    Back", "");
        return;
    }

    // Ready
    const s32 top = contentTop(), bottom = contentBottom();
    drawImage(r, top, bottom);

    char status[64];
    snprintf(status, sizeof(status), "%s  %.2fx", pageBuf, m_scale * m_img.factor);
    ui::drawHeader(r, name, status);

    char row2[96];
    snprintf(row2, sizeof(row2), " ±10 pages    %s    Legend    Back",
             m_fitWhole ? "Fit width" : "Fit page");
    ui::drawFooter(r, m_showHelp,
        " Pan    Zoom    Reset    Page",
        row2);

    const float sh = m_img.h * m_scale;
    if (sh > (bottom - top)) {
        const float frac = (top - m_oy) / (sh - (bottom - top));
        ui::drawProgress(r, top, bottom, std::clamp(frac, 0.0f, 1.0f), (bottom - top) / sh);
    }
}

// ---------------------------------------------------------------- input

bool GuiImageViewer::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState&, HidAnalogStickState, HidAnalogStickState) {
    if (keysDown & HidNpadButton_B) {
        tsl::goBack(); // the destructor saves the page
        return true;
    }

    for (u32 i = m_repR.update(keysHeld & HidNpadButton_R); i; --i)  goToPage(m_index + 1);
    for (u32 i = m_repL.update(keysHeld & HidNpadButton_L); i; --i)  goToPage(m_index - 1);
    for (u32 i = m_repZR.update(keysHeld & HidNpadButton_ZR); i; --i) goToPage(m_index + 10);
    for (u32 i = m_repZL.update(keysHeld & HidNpadButton_ZL); i; --i) goToPage(m_index - 10);
    if (keysDown & HidNpadButton_Minus) m_showHelp = !m_showHelp;

    if (m_state != State::Ready) return true;

    bool moved = false;
    if (keysHeld & input::NAV_LEFT)  { m_ox += PAN_SPEED; moved = true; }
    if (keysHeld & input::NAV_RIGHT) { m_ox -= PAN_SPEED; moved = true; }
    if (keysHeld & input::NAV_UP)    { m_oy += PAN_SPEED; moved = true; }
    if (keysHeld & input::NAV_DOWN)  { m_oy -= PAN_SPEED; moved = true; }
    if (moved) clampView();

    if (keysHeld & HidNpadButton_StickRUp)   zoomBy(ZOOM_STEP);
    if (keysHeld & HidNpadButton_StickRDown) zoomBy(1.0f / ZOOM_STEP);
    if (keysDown & HidNpadButton_StickR)     { m_fitWhole = false; fitWidth(); }
    if (keysDown & HidNpadButton_X) {
        m_fitWhole = !m_fitWhole;
        if (m_fitWhole) fitPage(); else fitWidth();
    }
    return true;
}
