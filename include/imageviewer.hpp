//imageviewer.hpp
#pragma once
#include <tesla.hpp>
#include <string>
#include <memory>

// Forward declaration of ImageViewerGui
class ImageViewerGui;

class ImageViewer : public tsl::Gui {
public:
    // Constructor
    ImageViewer(const std::string& path);

    // Destructor
    ~ImageViewer();

    // Create the UI for the image viewer
    tsl::elm::Element* createUI() override;

private:
    // Load the image from the given path
    void loadImage();

    // Path to the image file
    std::string m_path;

    // Pointer to the image pixel data
    uint8_t* m_pixels;

    // Image dimensions
    int m_width;
    int m_height;

    // Number of channels in the image
    int m_channels;

    // Flag indicating whether the image was loaded successfully
    bool m_loaded;

    // Scale factor for displaying the image
    float m_scale;

    // Offset for panning the image
    int m_offsetX;
    int m_offsetY;
};

// ImageViewerGui class definition
class ImageViewerGui : public tsl::elm::Element {
public:
    // Constructor
    ImageViewerGui(uint8_t* pixels, int width, int height, int channels,
        bool& loaded, float& scale, int& offsetX, int& offsetY);

    // Draw the image on the screen
    void draw(tsl::gfx::Renderer* renderer) override;

    // Layout the element within its parent
    void layout(u16 parentX, u16 parentY, u16 parentW, u16 parentH) override;

    // Handle user input
    bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touchPos,
        HidAnalogStickState joyStickPosLeft,
        HidAnalogStickState joyStickPosRight) override;

private:
    // Pointer to the image pixel data
    uint8_t* m_pixels;

    // Image dimensions
    int m_width;
    int m_height;

    // Number of channels in the image
    int m_channels;

    // Reference to the loaded flag
    bool& m_loaded;

    // Reference to the scale factor
    float& m_scale;

    // Reference to the offset for panning
    int& m_offsetX;
    int& m_offsetY;

    // Position and size within the parent
    u16 m_x, m_y, m_w, m_h;
};
