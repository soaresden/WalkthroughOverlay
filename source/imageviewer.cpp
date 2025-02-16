//imageviewer.cpp
#define STB_IMAGE_IMPLEMENTATION
#include "imageviewer.hpp"
#include <stb_image.h>
#include <memory>


ImageViewer::ImageViewer(const std::string& path)
    : m_path(path), m_pixels(nullptr), m_width(0), m_height(0), m_channels(0),
    m_loaded(false), m_scale(1.0f), m_offsetX(0), m_offsetY(0) {
    loadImage();
}

ImageViewer::~ImageViewer() {
    if (m_pixels) {
        stbi_image_free(m_pixels);
    }
}

void ImageViewer::loadImage() {
    m_pixels = stbi_load(m_path.c_str(), &m_width, &m_height, &m_channels, STBI_rgb_alpha);
    if (m_pixels) {
        m_loaded = true;
        float scaleX = (float)tsl::cfg::FramebufferWidth / m_width;
        float scaleY = (float)tsl::cfg::FramebufferHeight / m_height;
        m_scale = std::min(scaleX, scaleY);
    }
}

tsl::elm::Element* ImageViewer::createUI() {
    auto frame = new tsl::elm::OverlayFrame("Image Viewer", "");
    auto viewer = new ImageViewerGui(m_pixels, m_width, m_height, m_channels,
        m_loaded, m_scale, m_offsetX, m_offsetY);
    frame->setContent(viewer);
    return frame;
}

ImageViewerGui::ImageViewerGui(uint8_t* pixels, int width, int height, int channels,
    bool& loaded, float& scale, int& offsetX, int& offsetY)
    : m_pixels(pixels), m_width(width), m_height(height), m_channels(channels),
    m_loaded(loaded), m_scale(scale), m_offsetX(offsetX), m_offsetY(offsetY),
    m_x(0), m_y(0), m_w(0), m_h(0) {}

void ImageViewerGui::draw(tsl::gfx::Renderer* renderer) {
    if (!m_loaded) {
        renderer->drawString("Failed to load image", false, 20, 50, 16, renderer->a(0xFFFF));
        return;
    }

    int displayWidth = m_width * m_scale;
    int displayHeight = m_height * m_scale;

    int centerX = (m_w - displayWidth) / 2;
    int centerY = (m_h - displayHeight) / 2;

    int finalX = m_x + centerX + m_offsetX;
    int finalY = m_y + centerY + m_offsetY;

    // Correct the order and types of arguments
    renderer->drawBitmap(finalX, finalY, displayWidth, displayHeight, m_pixels);
}


void ImageViewerGui::layout(u16 parentX, u16 parentY, u16 parentW, u16 parentH) {
    m_x = parentX;
    m_y = parentY;
    m_w = parentW;
    m_h = parentH;
}

bool ImageViewerGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touchPos,
    HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    const float MOVE_SPEED = 10.0f;
    const float ZOOM_SPEED = 0.1f;

    if (keysHeld & HidNpadButton_Up)
        m_scale += ZOOM_SPEED;
    if (keysHeld & HidNpadButton_Down)
        m_scale -= ZOOM_SPEED;

    if (keysHeld & HidNpadButton_StickLLeft)
        m_offsetX -= MOVE_SPEED;
    if (keysHeld & HidNpadButton_StickLRight)
        m_offsetX += MOVE_SPEED;
    if (keysHeld & HidNpadButton_StickLUp)
        m_offsetY -= MOVE_SPEED;
    if (keysHeld & HidNpadButton_StickLDown)
        m_offsetY += MOVE_SPEED;

    if (keysDown & HidNpadButton_Y) {
        m_scale = 1.0f;
        m_offsetX = 0;
        m_offsetY = 0;
    }

    if (keysDown & HidNpadButton_B)
        tsl::goBack();

    return true;
}
