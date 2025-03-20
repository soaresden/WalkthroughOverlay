//StbImageElement.hpp
#pragma once
#include <tesla.hpp>
#include <string>

class StbImageElement : public tsl::elm::Element {
public:
    // Constructeur et destructeur
    StbImageElement(const std::string& path);
    virtual ~StbImageElement();

    // Impl�mentation des m�thodes de tsl::elm::Element
    virtual void draw(tsl::gfx::Renderer* renderer) override;
    virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touchPos,
        HidAnalogStickState leftJoystick, HidAnalogStickState rightJoystick) override;

private:
    // �tat de chargement de l'image
    enum class LoadState {
        NotStarted,
        Loading,
        Ready,
        Error
    };

    // M�thode de chargement d'image
    void loadImage();

    // Donn�es de l'image
    std::string m_path;
    std::string m_errorMessage;
    unsigned char* m_imageData;

    // Dimensions
    int m_width;
    int m_height;
    int m_originalWidth;
    int m_originalHeight;

    // Param�tres d'affichage
    float m_zoomLevel;
    int m_offsetX;
    int m_offsetY;

    // �tat
    LoadState m_state;
};