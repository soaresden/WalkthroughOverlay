// GuiImageViewer.hpp — full-screen image / page viewer.
//
// All images of the folder are treated as the pages of one document (that is what
// the PC converter produces from a PDF), naturally sorted: page_2 before page_10.
//
//  Left stick / D-pad  : pan
//  Right stick up/down : zoom in / out,  right stick click : back to fit-width
//  X                   : toggle fit-width / fit-whole-page
//  L / R               : previous / next page
//  ZL / ZR             : 10 pages back / forward
//  Minus               : show / hide the key legend
//  B                   : back (last page is remembered per folder)
#pragma once
#include <tesla.hpp>
#include <string>
#include <vector>
#include "ImageLoader.hpp"
#include "Input.hpp"

class GuiImageViewer : public tsl::Gui {
public:
    explicit GuiImageViewer(const std::string& path);
    ~GuiImageViewer() override;

    tsl::elm::Element* createUI() override;
    bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touch,
                     HidAnalogStickState left, HidAnalogStickState right) override;

private:
    enum class State { NeedLoad, Loading, Ready, Failed };

    void draw(tsl::gfx::Renderer* r);
    void drawImage(tsl::gfx::Renderer* r, s32 top, s32 bottom);
    void loadCurrent();
    void goToPage(s32 index);
    void fitWidth();
    void fitPage();
    void zoomBy(float k);
    void clampView();
    void saveResume();

    s32 contentTop() const;
    s32 contentBottom() const;

    std::string m_dir;
    std::vector<std::string> m_pages;
    s32 m_index = 0;

    Image m_img;
    State m_state = State::NeedLoad;
    u8 m_loadFrames = 0;
    std::string m_loadInfo;   // "3.1 MB free" shown while loading

    float m_scale = 1.0f;     // screen pixels per image pixel
    float m_ox = 0, m_oy = 0; // screen position of the image's top-left corner
    bool  m_fitWhole = false;
    bool  m_showHelp;

    input::Repeater m_repL{350, 200, 100000, 200}, m_repR{350, 200, 100000, 200};
    input::Repeater m_repZL{350, 250, 100000, 250}, m_repZR{350, 250, 100000, 250};
};
