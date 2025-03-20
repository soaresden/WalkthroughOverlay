// Fichier: ImageViewer.cpp
#include "ImageViewer.hpp"
#include "StbImageElement.hpp"

ImageViewer::ImageViewer(const std::string& path) : m_path(path) {}

tsl::elm::Element* ImageViewer::createUI() {
    auto* frame = new tsl::elm::OverlayFrame("Image Viewer", m_path);
    frame->setContent(new StbImageElement(m_path));
    return frame;
}