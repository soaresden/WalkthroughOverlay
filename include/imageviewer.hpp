//imageviewer.hpp

#pragma once
#include <tesla.hpp>
#include <string>
#include "StbImageElement.hpp"

class ImageViewer : public tsl::Gui {
public:
    ImageViewer(const std::string& path);
    virtual ~ImageViewer() = default;

    virtual tsl::elm::Element* createUI() override;

private:
    std::string m_path;
};