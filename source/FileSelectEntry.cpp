//FileSelectEntry.cpp
#include "FileSelectEntry.hpp"
#include "pdfviewer.hpp"
#include <Config.hpp>
#include "imageviewer.hpp"  // Ensure image handling is included

void FileSelectEntry::toggleFavorite() {
    m_favorite = !m_favorite;
    Config::update([this](json& j) {
        if (!j["favorites"].is_array()) {
            j["favorites"] = {};
        }
        if (m_favorite) {
            j["favorites"].push_back(getPath());
        }
        else {
            auto found = std::find(j["favorites"].begin(), j["favorites"].end(), getPath());
            if (found != j["favorites"].end()) {
                j["favorites"].erase(found);
            }
        }
        });
}

bool FileSelectEntry::operator<(FileSelectEntry const& other) const {
    if (ordering() < other.ordering())
        return true;
    else if (other.ordering() < ordering())
        return false;
    else
        return utils::compareIgnoreCase(m_path, other.m_path);
}

FileType FileSelectEntry::getFileType() const {
    std::string extension = getPathInternal().extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    // Images
    if (extension == ".jpg" || extension == ".jpeg" ||
        extension == ".png" || extension == ".bmp" ||
        extension == ".gif" || extension == ".tga") {
        return FileType::IMAGE;
    }

    // PDF
    if (extension == ".pdf") {
        return FileType::PDF;
    }

    // Text files
    if (extension == ".txt" || extension == ".log" ||
        extension == ".ini" || extension == ".cfg" ||
        extension == ".json" || extension == ".xml" ||
        extension == ".html" || extension == ".cpp" ||
        extension == ".h" || extension == ".hpp") {
        return FileType::TEXT;
    }

    return FileType::UNKNOWN;
}

void FileSelectFileEntry::select() {
    FileType type = getFileType();

    switch (type) {
    case FileType::TEXT:
        tsl::changeTo<TextReader>(getPath());
        break;

    case FileType::IMAGE:
        tsl::changeTo<ImageViewer>(getPath());
        break;

    case FileType::PDF:
        // TODO: Implement PDF viewer
        break;

    case FileType::UNKNOWN:
        // Default to opening as a text file
        tsl::changeTo<TextReader>(getPath());
        break;
    }
}

int FileSelectDirEntry::ordering() const {
    return 1;  // Prioritize directories
}

int FileSelectFileEntry::ordering() const {
    return 2;  // Prioritize files
}
